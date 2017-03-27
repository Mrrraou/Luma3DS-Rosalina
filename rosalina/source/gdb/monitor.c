#include "gdb/monitor.h"
#include "gdb/net.h"
#include "gdb/debug.h"

extern Handle terminationRequestEvent;
extern bool terminationRequest;

void GDB_RunMonitor(GDBServer *server)
{
    Handle handles[3 + 2 * MAX_DEBUG];
    Result r = 0;

    handles[0] = terminationRequestEvent;
    handles[1] = server->super.shall_terminate_event;
    handles[2] = server->statusUpdated;

    do
    {
        u32 nbProcessHandles = 0;
        for(int i = 0; i < MAX_DEBUG; i++)
        {
            GDBContext *ctx = &server->ctxs[i];
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
            GDBContext *ctx = &server->ctxs[idx - 3];

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
            for(i = 0; i < MAX_DEBUG && server->ctxs[i].process != handles[idx]; i++);
            if(i == MAX_DEBUG)
                continue;

            GDBContext *ctx = &server->ctxs[i];
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
    while(!terminationRequest && server->super.running);
}
