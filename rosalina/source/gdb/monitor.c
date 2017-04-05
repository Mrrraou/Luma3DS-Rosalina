#include "gdb/monitor.h"
#include "gdb/net.h"
#include "gdb/debug.h"

extern Handle terminationRequestEvent;
extern bool terminationRequest;

void GDB_RunMonitor(GDBServer *server)
{
    Handle handles[3 + MAX_DEBUG];
    Result r = 0;

    handles[0] = terminationRequestEvent;
    handles[1] = server->super.shall_terminate_event;
    handles[2] = server->statusUpdated;

    do
    {
        for(int i = 0; i < MAX_DEBUG; i++)
        {
            GDBContext *ctx = &server->ctxs[i];
            handles[3 + i] = ctx->eventToWaitFor;
        }

        s32 idx = -1;
        r = svcWaitSynchronizationN(&idx, handles, 3 + MAX_DEBUG, false, -1LL);

        if(R_FAILED(r) || idx < 2)
            break;
        else if(idx == 2)
            continue;
        else
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
                int res = GDB_HandleDebugEvents(ctx);
                if(res >= 0)
                    ctx->eventToWaitFor = ctx->continuedEvent;
                else if(res == -2)
                {
                    while(GDB_HandleDebugEvents(ctx) != -1) // until we've got all the remaining debug events
                        svcSleepThread(1 * 1000 * 1000LL); // sleep just in case

                    svcClearEvent(ctx->clientAcceptedEvent);
                    ctx->eventToWaitFor = ctx->clientAcceptedEvent;
                }
            }

            RecursiveLock_Unlock(&ctx->lock);
        }
    }
    while(!terminationRequest && server->super.running);
}
