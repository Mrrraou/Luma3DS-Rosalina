#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
#include "gdb/server.h"
#include "gdb/debug.h"

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

void Debugger_Enable(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    if(gdbServer.running)
    {
        draw_string("Already enabled!", 10, 10, COLOR_TITLE);
    }
    else
    {
        for(int i = 0; i < MAX_DEBUG; i++)
            GDB_InitializeContext(gdbCtxs + i);

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
            server_init(&gdbServer);
            gdbServer.running = true;
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

    gdbServer.running = false;
    MyThread_Join(&debuggerSocketThread, -1);
    MyThread_Join(&debuggerDebugThread, -1);

    miniSocExit();
    draw_string("Debugger disabled.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void debuggerSocketThreadMain(void)
{
    gdbCtxs[0].pid = 0x10;
    server_bind(&gdbServer, 4000);
    server_bind(&gdbServer, 4001);
    server_bind(&gdbServer, 4002);
    server_run(&gdbServer);
}

void debuggerDebugThreadMain(void)
{
    Handle handles[1 + MAX_DEBUG];

    Result r = 0;

    handles[0] = terminationRequestEvent;

    while(!terminationRequest && gdbServer.running)
    {
        for(int i = 0; i < MAX_DEBUG; i++)
            handles[1 + i] = gdbCtxs[i].eventToWaitFor;

        s32 idx = -1;
        r = svcWaitSynchronizationN(&idx, handles, 1 + MAX_DEBUG, false, -1LL);

        if(R_SUCCEEDED(r) && idx == 0)
            break;
        else if(R_FAILED(r) || idx < 0)
            continue;
        else
        {
            GDBContext *ctx = &gdbCtxs[0];
            //RecursiveLock_Lock(&ctx->lock);

            if(ctx->eventToWaitFor == ctx->clientAcceptedEvent)
                ctx->eventToWaitFor = ctx->continuedEvent;
            else if(ctx->eventToWaitFor == ctx->continuedEvent)
                ctx->eventToWaitFor = ctx->debug;
            else
            {
                RecursiveLock_Lock(&ctx->lock);
                ctx->eventToWaitFor = ctx->continuedEvent;
                GDB_HandleDebugEvents(ctx);
                RecursiveLock_Unlock(&ctx->lock);
            }

            //RecursiveLock_Unlock(&ctx->lock);
        }
    }
}
