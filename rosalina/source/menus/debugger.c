#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
#include "gdb_ctx.h"
#include "sock_util.h"
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
    //gdb_server.close_cb = gdb_close;

    gdb_server.alloc = gdb_get_client;
    gdb_server.free = gdb_release_client;

    void *c = NULL;
    for(int i = 0; i < MAX_DEBUG; i++)
    {
        if(!(gdb_server_ctxs[i].flags & GDB_FLAG_USED))
        {
            gdb_server_ctxs[i].flags |= GDB_FLAG_USED;
            c = &gdb_server_ctxs[i];
        }
    }

    server_bind(&gdb_server, 4000, c);
    server_run(&gdb_server);
}

void debuggerDebugThreadMain(void)
{
}