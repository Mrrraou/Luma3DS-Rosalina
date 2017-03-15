#include "gdb/server.h"
#include "gdb/net.h"
#include "gdb/query.h"
#include "gdb/long_command.h"
#include "gdb/thread.h"
#include "gdb/debug.h"
#include "gdb/regs.h"
#include "gdb/mem.h"

void GDB_InitializeServer(GDBServer *server)
{
    server_init(&server->super);

    server->super.host = 0;

    server->super.accept_cb = GDB_AcceptClient;
    server->super.data_cb   = GDB_DoPacket;
    server->super.close_cb  = GDB_CloseClient;

    server->super.alloc     = GDB_GetClient;
    server->super.free      = GDB_ReleaseClient;

    server->super.clients_per_server = 1;

    svcCreateEvent(&server->statusUpdated, RESET_ONESHOT);

    for(u32 i = 0; i < sizeof(server->ctxs) / sizeof(GDBContext); i++)
        GDB_InitializeContext(server->ctxs + i);
}

void GDB_FinalizeServer(GDBServer *server)
{
    //server_finalize(&server->super);

    svcCloseHandle(server->statusUpdated);

    for(u32 i = 0; i < sizeof(server->ctxs) / sizeof(GDBContext); i++)
        GDB_FinalizeContext(server->ctxs + i);
}

#ifndef GDB_PORT_BASE
#define GDB_PORT_BASE 4000
#endif

void GDB_RunServer(GDBServer *server)
{
    server->ctxs[0].pid = 0x10;
    server_bind(&server->super, GDB_PORT_BASE);
    server_bind(&server->super, GDB_PORT_BASE + 1);
    server_bind(&server->super, GDB_PORT_BASE + 2);
    server_run(&server->super);
}

void GDB_StopServer(GDBServer *server)
{
    server_stop(&server->super);
}

int GDB_AcceptClient(sock_ctx *socketCtx)
{
    GDBContext *ctx = (GDBContext *)socketCtx;

    RecursiveLock_Lock(&ctx->lock);

    Result r = svcOpenProcess(&ctx->process, ctx->pid);
    if(R_SUCCEEDED(r))
    {
        r = svcDebugActiveProcess(&ctx->debug, ctx->pid);
        if(R_SUCCEEDED(r))
        {
            ctx->state = GDB_STATE_CONNECTED;
            while(R_SUCCEEDED(svcGetProcessDebugEvent(&ctx->latestDebugEvent, ctx->debug)));
        }
        else
        {
            //TODO: clean up
            svcCloseHandle(ctx->process);
        }
    }

    RecursiveLock_Unlock(&ctx->lock);

    svcSignalEvent(ctx->clientAcceptedEvent);
    return 0;
}

int GDB_CloseClient(sock_ctx *socketCtx)
{
    GDBContext *ctx = (GDBContext *)socketCtx;

    RecursiveLock_Lock(&ctx->lock);
    svcClearEvent(ctx->clientAcceptedEvent);
    ctx->eventToWaitFor = ctx->clientAcceptedEvent;
    RecursiveLock_Unlock(&ctx->lock);

    DebugEventInfo dummy;
    while(R_SUCCEEDED(svcGetProcessDebugEvent(&dummy, ctx->debug)));
    while(R_SUCCEEDED(svcContinueDebugEvent(ctx->debug, (DebugFlags)0)));

    return 0;
}

sock_ctx *GDB_GetClient(sock_server *socketSrv)
{
    GDBServer *server = (GDBServer *)socketSrv;

    for(u32 i = 0; i < sizeof(server->ctxs) / sizeof(GDBContext); i++)
    {
        if(!(server->ctxs[i].flags & GDB_FLAG_USED))
        {
            RecursiveLock_Lock(&server->ctxs[i].lock);
            server->ctxs[i].flags |= GDB_FLAG_USED;
            server->ctxs[i].state = GDB_STATE_CONNECTED;
            RecursiveLock_Unlock(&server->ctxs[i].lock);

            return &server->ctxs[i].super;
        }
    }

    return NULL;
}

void GDB_ReleaseClient(sock_server *socketSrv, sock_ctx *socketCtx)
{
    GDBContext *ctx = (GDBContext *)socketCtx;
    GDBServer *server = (GDBServer *)socketSrv;

    svcSignalEvent(server->statusUpdated);

    RecursiveLock_Lock(&ctx->lock);

    svcCloseHandle(ctx->debug);
    svcCloseHandle(ctx->process);
    ctx->flags = (GDBFlags)0;
    ctx->state = GDB_STATE_DISCONNECTED;

    ctx->eventToWaitFor = ctx->clientAcceptedEvent;
    ctx->pid = 0;
    ctx->currentThreadId = ctx->selectedThreadId = 0;

    ctx->catchThreadEvents = ctx->processExited = false;
    ctx->nbPendingDebugEvents = ctx->nbDebugEvents = 0;

    RecursiveLock_Unlock(&ctx->lock);
}

static inline GDBCommandHandler GDB_GetCommandHandler(char c)
{
    switch(c)
    {
        case 'q':
            return GDB_HandleReadQuery;

        case 'Q':
            return GDB_HandleWriteQuery;

        case 'v':
            return GDB_HandleLongCommand;

        case '?':
            return GDB_HandleGetStopReason;

        case 'g':
            return GDB_HandleReadRegisters;

        case 'G':
            return GDB_HandleWriteRegisters;

        case 'p':
            return GDB_HandleReadRegister;

        case 'P':
            return GDB_HandleWriteRegister;

        case 'm':
            return GDB_HandleReadMemory;

        case 'M':
            return GDB_HandleWriteMemory;

        case 'X':
            return GDB_HandleWriteMemoryRaw;

        case 'H':
            return GDB_HandleSetThreadId;

        case 'T':
            return GDB_HandleIsThreadAlive;

        case 'c':
        case 'C':
            return GDB_HandleContinue;

        case 'D':
            return GDB_HandleDetach;

        case 'k':
            return GDB_HandleKill;

        default:
            return GDB_HandleUnsupported;
    }
}

int GDB_DoPacket(sock_ctx *socketCtx)
{
    int ret;
    Handle socket = socketCtx->sock;
    GDBContext *ctx = (GDBContext *)socketCtx;

    RecursiveLock_Lock(&ctx->lock);
    GDBFlags oldFlags = ctx->flags;

    switch(ctx->state)
    {
        // Both of these require a '+'.
        case GDB_STATE_CONNECTED:
        case GDB_STATE_NOACK_SENT:
        {
            int r = soc_recv(socket, ctx->buffer, 1, 0);
            if(r != 1)
            {
                ret = -1;
                goto unlock;
            }
            else
            {
                if(ctx->buffer[0] != '+')
                {
                    ret = -1;
                    goto unlock;
                }
                else
                {
                    if(ctx->state == GDB_STATE_NOACK_SENT)
                    {
                        ctx->state = GDB_STATE_NOACK;
                        ret = 0;
                        goto unlock;
                    }
                }
            }

            r = soc_send(socket, "+", 1, 0); // Yes. :(
            if(r == -1)
            {
                ret = -1;
                goto unlock;
            }
        }

        // lack of break is intentional
        case GDB_STATE_NOACK:
        {
            char cksum[3];
            cksum[2] = 0;
            int r = soc_recv_until(socket, ctx->buffer, GDB_BUF_LEN, "#", 1, true);

            // Bubbling -1 up to server will close the connection.
            if(r <= 0)
            {
                ret = -1;
                goto unlock;
            }

            else if(ctx->buffer[0] == '\x03')
            {
                GDB_HandleBreak(ctx);
                ret = 0;
                goto unlock;
            }

            else
            {
                soc_recv(socket, cksum, 2, 0);

                ctx->buffer[r-1] = 0; // replace trailing '#' with 0

                GDBCommandHandler handler = GDB_GetCommandHandler(ctx->buffer[1]);
                if(handler != NULL)
                {
                    ctx->commandData = ctx->buffer + 2;
                    int res = handler(ctx);
                    if(res == -1)
                    {
                        ret = GDB_ReplyEmpty(ctx); // Handler failed!
                        goto unlock;
                    }
                    ret = res;
                }
                else
                    ret = GDB_HandleUnsupported(ctx); // We don't have a handler!
            }

            break;
        }

        default:
            break;
    }

    unlock:
    RecursiveLock_Unlock(&ctx->lock);
    if(ctx->state == GDB_STATE_CLOSING)
    {
        if(ctx->flags & GDB_FLAG_TERMINATE_PROCESS)
        {
            GDB_BreakProcessAndSinkDebugEvents(ctx, DBG_INHIBIT_USER_CPU_EXCEPTION_HANDLERS);
            svcTerminateDebugProcess(ctx->debug);
        }

        return -1;
    }

    if((oldFlags & GDB_FLAG_PROCESS_CONTINUING) && !(ctx->flags & GDB_FLAG_PROCESS_CONTINUING))
    {
        if(R_FAILED(svcBreakDebugProcess(ctx->debug)))
            ctx->flags |= GDB_FLAG_PROCESS_CONTINUING;
    }
    else if(!(oldFlags & GDB_FLAG_PROCESS_CONTINUING) && (ctx->flags & GDB_FLAG_PROCESS_CONTINUING))
        svcSignalEvent(ctx->continuedEvent);

    return ret;
}
