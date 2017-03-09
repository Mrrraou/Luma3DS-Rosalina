#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
#include "gdb_ctx.h"
#include "sock_util.h"
#include "macros.h"
#include <sys/socket.h>

Menu menu_debugger = {
    "Debugger options menu",
    .items = 2,
    {
        {"Enable debugger", METHOD, .method = &Debugger_Enable},
        {"Disable debugger", METHOD, .method = &Debugger_Disable}
    }
};

static MyThread debuggerSocketThread;
static MyThread debuggerDebugThread;
static u8 ALIGN(8) debuggerSocketThreadStack[THREAD_STACK_SIZE];
static u8 ALIGN(8) debuggerDebugThreadStack[THREAD_STACK_SIZE];

struct sock_server gdb_server;
struct gdb_client_ctx gdb_client_ctxs[MAX_CLIENTS];
struct gdb_server_ctx gdb_server_ctxs[MAX_DEBUG];

void debuggerSocketThreadMain(void);
MyThread *debuggerCreateSocketThread(void)
{
    MyThread_Create(&debuggerSocketThread, debuggerSocketThreadMain, debuggerSocketThreadStack, THREAD_STACK_SIZE, 0x20, CORE_SYSTEM);
    return &debuggerSocketThread;
}

void debuggerDebugThreadMain(void);
MyThread *debuggerCreateDebugThread(void)
{
    MyThread_Create(&debuggerDebugThread, debuggerDebugThreadMain, debuggerDebugThreadStack, THREAD_STACK_SIZE, 0x20, CORE_SYSTEM);
    return &debuggerDebugThread;
}

static Handle attachEvent;
void Debugger_Enable(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    if(gdb_server.running)
    {
        draw_string("Already enabled!", 10, 10, COLOR_TITLE);
    }
    else
    {
        for(int i = 0; i < MAX_DEBUG; i++)
        {
            svcCreateEvent(&gdb_server_ctxs[i].continuedEvent, RESET_ONESHOT);
            svcCreateEvent(&gdb_server_ctxs[i].clientAcceptedEvent, RESET_STICKY);
        }

        svcCreateEvent(&attachEvent, RESET_ONESHOT);
        draw_string("Initialising SOC...", 10, 10, COLOR_WHITE);

        s64 amt = 0;
        svcGetSystemInfo(&amt, 0, 2);

        char msg1[] = "00000000 free in SYSTEM";
        hexItoa(0x02C00000 - amt, msg1, 8, false);
        draw_string(msg1, 10, 20, COLOR_WHITE);

        Result res = miniSocInit(0x20000);
        if(R_FAILED(res))
        {
            char msg2[] = "00000000 err";
            hexItoa(res, msg2, 8, false);
            draw_string(msg2, 10, 30, COLOR_WHITE);
        }
        else
        {
            server_init(&gdb_server);
            gdb_server.running = true;
            debuggerCreateSocketThread();
            debuggerCreateDebugThread();
            draw_string("Debugger thread started successfully.", 10, 10, COLOR_TITLE);
        }
    }

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void Debugger_Disable(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    gdb_server.running = false;
    MyThread_Join(&debuggerSocketThread, -1);
    MyThread_Join(&debuggerDebugThread, -1);

    miniSocExit();
    draw_string("Debugger disabled.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void debuggerSocketThreadMain(void)
{
    gdb_server.userdata = gdb_client_ctxs;
    gdb_server.host = 0;

    gdb_server.accept_cb = gdb_accept_client;
    gdb_server.data_cb = gdb_do_packet;
    gdb_server.close_cb = gdb_close_client;

    gdb_server.alloc = gdb_get_client;
    gdb_server.free = gdb_release_client;

    gdb_server.clients_per_server = 1;

    debugger_attach(&gdb_server, 0x10);
    server_run(&gdb_server);
}

void debuggerDebugThreadMain(void)
{
    Handle handles[2 + MAX_DEBUG];
    struct gdb_server_ctx *mapping[MAX_DEBUG];

    int n = 0;
    Result r = 0;

    handles[0] = terminationRequestEvent;
    handles[1] = attachEvent;

    while(!terminationRequest && gdb_server.running)
    {
        n = 0;

        for(int i = 0; i < MAX_DEBUG; i++)
        {
            if(gdb_server_ctxs[i].flags & GDB_FLAG_USED)
            {
                if(gdb_server_ctxs[i].debug != 0)
                {
                    mapping[n] = &gdb_server_ctxs[i];
                    handles[2 + n++] = gdb_server_ctxs[i].debugOrContinuedEvent;
                }
            }
        }

        s32 idx = -1;
        r = svcWaitSynchronizationN(&idx, handles, 2 + n, false, -1LL);
        if(R_SUCCEEDED(r) && idx == 0)
            break;
        else if(R_FAILED(r) || idx == 1)
            continue;
        else
        {
            struct gdb_server_ctx *serv_ctx = mapping[idx - 2];
            svcWaitSynchronization(serv_ctx->clientAcceptedEvent, -1LL);
            struct sock_ctx *client_ctx = serv_ctx->client;
            struct gdb_client_ctx *client_gdb_ctx = serv_ctx->client_gdb_ctx;

            if(serv_ctx->debugOrContinuedEvent == serv_ctx->continuedEvent)
                serv_ctx->debugOrContinuedEvent = serv_ctx->debug;
            if(client_gdb_ctx)
            {
                serv_ctx->debugOrContinuedEvent = serv_ctx->continuedEvent;
                RecursiveLock_Lock(&client_gdb_ctx->sock_lock);
                gdb_handle_debug_events(handles[idx], client_ctx);
                RecursiveLock_Unlock(&client_gdb_ctx->sock_lock);
            }
        }
    }
}

Result debugger_attach(struct sock_server *serv, u32 pid)
{
    struct gdb_server_ctx *c = NULL;
    for(int i = 0; i < MAX_DEBUG; i++)
    {
        if(!(gdb_server_ctxs[i].flags & GDB_FLAG_USED))
            c = &gdb_server_ctxs[i];
    }

    if(c == NULL) return -1; // no slot?

    Result r = svcOpenProcess(&c->proc, pid);
    if(R_SUCCEEDED(r))
    {
        r = svcDebugActiveProcess(&c->debug, pid);
        if(R_SUCCEEDED(r))
        {
            while(R_SUCCEEDED(svcGetProcessDebugEvent(&c->latestDebugEvent, c->debug)))
            {
                if(c->latestDebugEvent.type != DBGEVENT_EXCEPTION || c->latestDebugEvent.exception.type != EXCEVENT_ATTACH_BREAK)
                    svcContinueDebugEvent(c->debug, DBG_NO_ERRF_CPU_EXCEPTION_DUMPS);
            }
            server_bind(serv, 4000 + pid, c);
            c->debugOrContinuedEvent = c->continuedEvent;
            c->flags |= GDB_FLAG_USED;
            svcSignalEvent(attachEvent);
        }
        else
        {
            //TODO: server_unbind
            svcCloseHandle(c->proc);
        }
    }

    return r;
}

// TODO: stub
Result debugger_detach(struct sock_server *serv UNUSED, u32 pid UNUSED)
{
    return 0;
}
