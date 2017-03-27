#include "menus/debugger.h"
#include "memory.h"
#include "draw.h"
#include "minisoc.h"
#include "gdb/server.h"
#include "gdb/debug.h"
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
    Handle handles[3 + 2 * MAX_DEBUG];

    Result r = 0;

    GDB_IncrementServerReferenceCount(&server);

    handles[0] = terminationRequestEvent;
    handles[1] = server.super.shall_terminate_event;
    handles[2] = server.statusUpdated;

    do
    {
        u32 nbProcessHandles = 0;
        for(int i = 0; i < MAX_DEBUG; i++)
        {
            GDBContext *ctx = &server.ctxs[i];
            handles[3 + i] = ctx->eventToWaitFor;
            if(ctx->state != GDB_STATE_DISCONNECTED && ctx->state != GDB_STATE_CLOSING && !ctx->processEnded)
                handles[3 + MAX_DEBUG + nbProcessHandles++] = ctx->process;
        }

        s32 idx = -1;
        r = svcWaitSynchronizationN(&idx, handles, 3 + MAX_DEBUG + nbProcessHandles, false, -1LL);

        if(R_FAILED(r) || idx < 2)
            break;
        else if(idx == 2)
            continue;
        else if(idx < 3 + MAX_DEBUG)
        {
            GDBContext *ctx = &server.ctxs[idx - 3];

            RecursiveLock_Lock(&ctx->lock);
            if(ctx->state == GDB_STATE_DISCONNECTED || ctx->state == GDB_STATE_CLOSING)
            {
                svcClearEvent(ctx->clientAcceptedEvent);
                ctx->eventToWaitFor = ctx->clientAcceptedEvent;
                RecursiveLock_Unlock(&ctx->lock);
                continue;
            }

            if(ctx->eventToWaitFor == ctx->clientAcceptedEvent)
                ctx->eventToWaitFor = ctx->continuedEvent;
            else if(ctx->eventToWaitFor == ctx->continuedEvent)
                ctx->eventToWaitFor = ctx->debug;
            else
            {
                if(GDB_HandleDebugEvents(ctx) >= 0)
                    ctx->eventToWaitFor = ctx->continuedEvent;
            }

            RecursiveLock_Unlock(&ctx->lock);
        }
        else // process terminated
        {
            u32 i;
            for(i = 0; i < MAX_DEBUG && server.ctxs[i].process != handles[idx]; i++);
            if(i == MAX_DEBUG)
                continue;

            GDBContext *ctx = &server.ctxs[i];
            if(ctx->state == GDB_STATE_DISCONNECTED || ctx->state == GDB_STATE_CLOSING)
                continue;

            RecursiveLock_Lock(&ctx->lock);

            ctx->processEnded = true;
            ctx->catchThreadEvents = false; // don't report them

            if(!(ctx->flags & GDB_FLAG_PROCESS_CONTINUING))
            {
                // if the process ends when it's being debugged, it is because it has been terminated
                DebugEventInfo info;
                info.type = DBGEVENT_EXIT_PROCESS;
                info.exit_process.reason = EXITPROCESS_EVENT_TERMINATE;
                ctx->pendingDebugEvents[ctx->nbPendingDebugEvents++] = info;
            }

            else
            {
                svcSleepThread(25 * 1000 * 1000LL); // wait a bit and see if we can net some thread exit debug events

                for(u32 i = 0; i < 0x7F; i++)
                    GDB_HandleDebugEvents(ctx);
                GDB_SendPacket(ctx, ctx->processExited ? "W00" : "X0f", 3); // GDB will close the connection when receiving this
            }

            svcClearEvent(ctx->clientAcceptedEvent);
            ctx->eventToWaitFor = ctx->clientAcceptedEvent; // GDB will disconnect when receiving the termination signal

            RecursiveLock_Unlock(&ctx->lock);
        }
    }
    while(!terminationRequest && server.super.running);
    GDB_DecrementServerReferenceCount(&server);
}
