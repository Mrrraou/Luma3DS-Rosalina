#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
#include "gdb/server.h"
#include "gdb/debug.h"
#include "gdb/monitor.h"
#include "gdb/net.h"

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

static GDBServer server;

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

    if(server.super.running)
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

        Result res = GDB_InitializeServer(&server);

        if(R_FAILED(res))
        {
            char msg2[] = "00000000 err";
            hexItoa(res, msg2, 8, false);
            draw_string(msg2, 10, 30, COLOR_WHITE);
        }
        else
        {
            debuggerCreateSocketThread();
            debuggerCreateDebugThread();
            draw_string("Debugger started successfully.", 10, 10, COLOR_TITLE);
        }
    }

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void Debugger_Disable(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    svcSignalEvent(server.super.shall_terminate_event);
    MyThread_Join(&debuggerDebugThread, -1);
    MyThread_Join(&debuggerSocketThread, -1);

    draw_string("Debugger disabled.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void debuggerSocketThreadMain(void)
{
    GDB_IncrementServerReferenceCount(&server);
    GDB_RunServer(&server);
    GDB_DecrementServerReferenceCount(&server);
}

void debuggerDebugThreadMain(void)
{
    GDB_IncrementServerReferenceCount(&server);
    GDB_RunMonitor(&server);
    GDB_DecrementServerReferenceCount(&server);
}
