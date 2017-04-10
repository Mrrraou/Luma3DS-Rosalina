#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
#include "fmt.h"
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
static u8 ALIGN(8) debuggerSocketThreadStack[2 * THREAD_STACK_SIZE];
static u8 ALIGN(8) debuggerDebugThreadStack[2 * THREAD_STACK_SIZE];

GDBServer gdbServer = { 0 };

void debuggerSocketThreadMain(void);
MyThread *debuggerCreateSocketThread(void)
{
    MyThread_Create(&debuggerSocketThread, debuggerSocketThreadMain, debuggerSocketThreadStack, 2 * THREAD_STACK_SIZE, 0x20, CORE_SYSTEM);
    return &debuggerSocketThread;
}

void debuggerDebugThreadMain(void);
MyThread *debuggerCreateDebugThread(void)
{
    MyThread_Create(&debuggerDebugThread, debuggerDebugThreadMain, debuggerDebugThreadStack, 2 * THREAD_STACK_SIZE, 0x20, CORE_SYSTEM);
    return &debuggerDebugThread;
}

void Debugger_Enable(void)
{
    bool done = false, alreadyEnabled = gdbServer.super.running;
    Result res = 0;
    char buf[65];

    draw_lock();
    draw_clearFramebuffer();
    draw_flushFramebuffer();
    draw_unlock();

    do
    {
        draw_lock();
        draw_string("Debugger options menu", 10, 10, COLOR_TITLE);

        if(alreadyEnabled)
            draw_string("Already enabled!", 10, 30, COLOR_WHILE);

        else
        {
            draw_string("Starting debugger...", 10, 30, COLOR_WHITE);

            if(!done)
            {
                res = GDB_InitializeServer(&gdbServer);
                if(R_SUCCEEDED(res))
                {
                    debuggerCreateSocketThread();
                    debuggerCreateDebugThread();
                    svcWaitSynchronization(gdbServer.super.started_event, -1LL);
                }
                else
                    sprintf(buf, "Starting debugger... failed (0x%08x).", (u32)res);

                done = true;
            }
            if(R_SUCCEEDED(res))
                draw_string("Starting debugger... OK.", 10, 30, COLOR_WHITE);
            else
                draw_string(buf, 10, 30, COLOR_WHITE);

            s64 amt = 0;
            svcGetSystemInfo(&amt, 0, 2);

            char msg1[65];
            sprintf(msg1, "%d bytes free in SYSTEM", 0x02C00000LL - amt);

            draw_string(msg1, 10, 50, COLOR_WHITE);
        }

        draw_flushFramebuffer();
        draw_unlock();
    }
    while(!(waitInput() & BUTTON_B) && !terminationRequest);
}

void Debugger_Disable(void)
{
    bool initialized = gdbServer.referenceCount != 0;
    if(initialized)
    {
        svcSignalEvent(gdbServer.super.shall_terminate_event);
        MyThread_Join(&debuggerDebugThread, -1);
        MyThread_Join(&debuggerSocketThread, -1);
    }
    else
    draw_lock();
    draw_clearFramebuffer();
    draw_flushFramebuffer();
    draw_unlock();

    do
    {
        draw_lock();
        draw_string("Debugger options menu", 10, 10, COLOR_TITLE);
        draw_string(initialized ? "Debugger disabled." : "Debugger not enabled.", 10, 30, COLOR_WHITE);
        draw_flushFramebuffer();
        draw_unlock();
    }
    while(!(waitInput() & BUTTON_B) && !terminationRequest);
}

void debuggerSocketThreadMain(void)
{
    GDB_IncrementServerReferenceCount(&gdbServer);
    GDB_RunServer(&gdbServer);
    GDB_DecrementServerReferenceCount(&gdbServer);
}

void debuggerDebugThreadMain(void)
{
    GDB_IncrementServerReferenceCount(&gdbServer);
    GDB_RunMonitor(&gdbServer);
    GDB_DecrementServerReferenceCount(&gdbServer);
}
