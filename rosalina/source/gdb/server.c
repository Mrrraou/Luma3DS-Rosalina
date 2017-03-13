#include "gdb/server.h"
#include "gdb/net.h"
#include "gdb/query.h"
#include "gdb/long_command.h"
#include "gdb/thread.h"
#include "gdb/debug.h"
#include "gdb/regs.h"
#include "gdb/mem.h"

GDBContext  gdbCtxs[MAX_DEBUG];

sock_server gdbServer =
{
    .userdata  = (sock_ctx *)gdbCtxs,
    .host      = 0,

    .accept_cb = GDB_AcceptClient,
    .data_cb   = GDB_DoPacket,
    .close_cb  = GDB_CloseClient,

    .alloc     = GDB_GetClient,
    .free      = GDB_ReleaseClient,

    .clients_per_server = 1
};

static GDBCommandHandler GDB_GetCommandHandler(char c)
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

        default:
            return GDB_HandleUnsupported;
    }
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

int GDB_CloseClient(sock_ctx *socketCtx UNUSED)
{
    // should it call GDB_FinalizeContext ?
    return 0;
}

sock_ctx *GDB_GetClient(sock_server *socketSrv)
{
    GDBContext *ctxs = (GDBContext *)socketSrv->userdata;
    for(int i = 0; i < MAX_DEBUG; i++)
    {
        if(!(ctxs[i].flags & GDB_FLAG_USED))
        {
            RecursiveLock_Lock(&ctxs[i].lock);
            ctxs[i].flags |= GDB_FLAG_USED;
            ctxs[i].state = GDB_STATE_CONNECTED;
            RecursiveLock_Unlock(&ctxs[i].lock);

            return (sock_ctx *)&ctxs[i];
        }
    }

    return NULL;
}

void GDB_ReleaseClient(sock_server *socketSrv UNUSED, sock_ctx *socketCtx)
{
    GDBContext *ctx = (GDBContext *)socketCtx;

    RecursiveLock_Lock(&ctx->lock);
    ctx->flags &= ~GDB_FLAG_USED;
    svcClearEvent(ctx->clientAcceptedEvent);
    ctx->eventToWaitFor = ctx->clientAcceptedEvent;
    RecursiveLock_Unlock(&ctx->lock);
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
        }
        break;
    }

    unlock:
    RecursiveLock_Unlock(&ctx->lock);

    if((oldFlags & GDB_FLAG_PROCESS_CONTINUING) && !(ctx->flags & GDB_FLAG_PROCESS_CONTINUING))
    {
        if(R_FAILED(svcBreakDebugProcess(ctx->debug)))
            ctx->flags |= GDB_FLAG_PROCESS_CONTINUING;
    }
    else if(!(oldFlags & GDB_FLAG_PROCESS_CONTINUING) && (ctx->flags & GDB_FLAG_PROCESS_CONTINUING))
        svcSignalEvent(ctx->continuedEvent);

    return ret;
}
