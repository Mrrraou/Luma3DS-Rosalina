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

        Result res = miniSocInit(0x30000);
        if(R_FAILED(res))
        {
            char msg2[] = "00000000 err";
            hexItoa(res, msg2, 8, false);
            draw_string(msg2, 10, 30, COLOR_WHITE);
        }
        else
        {
            GDB_InitializeServer(&server);
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

    GDB_StopServer(&server);
    GDB_FinalizeServer(&server);

    MyThread_Join(&debuggerSocketThread, -1);
    MyThread_Join(&debuggerDebugThread, -1);

    miniSocExit();
    draw_string("Debugger disabled.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void debuggerSocketThreadMain(void)
{
    GDB_RunServer(&server);
}

void debuggerDebugThreadMain(void)
{
    Handle handles[2 + MAX_DEBUG];

    Result r = 0;

    handles[0] = terminationRequestEvent;
    handles[1] = server.statusUpdated;

    do
    {
        for(int i = 0; i < MAX_DEBUG; i++)
            handles[2 + i] = server.ctxs[i].eventToWaitFor;

        s32 idx = -1;
        r = svcWaitSynchronizationN(&idx, handles, 2 + MAX_DEBUG, false, -1LL);

        if(R_SUCCEEDED(r) && idx == 0)
            break;
        else if(idx < 2)
            continue;
        else
        {
            GDBContext *ctx = &server.ctxs[idx - 2];

            if(ctx->state == GDB_STATE_DISCONNECTED || ctx->state == GDB_STATE_CLOSING)
                continue;

            RecursiveLock_Lock(&ctx->lock);

            if(ctx->eventToWaitFor == ctx->clientAcceptedEvent)
                ctx->eventToWaitFor = ctx->continuedEvent;
            else if(ctx->eventToWaitFor == ctx->continuedEvent)
                ctx->eventToWaitFor = ctx->debug;
            else
            {
                GDB_HandleDebugEvents(ctx);
                ctx->eventToWaitFor = ctx->continuedEvent;
            }

            RecursiveLock_Unlock(&ctx->lock);
        }
    }
    while(!terminationRequest && server.super.running);
}
