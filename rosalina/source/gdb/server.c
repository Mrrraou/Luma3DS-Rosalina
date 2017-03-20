#include "gdb/server.h"
#include "gdb/net.h"
#include "gdb/query.h"
#include "gdb/verbose.h"
#include "gdb/thread.h"
#include "gdb/debug.h"
#include "gdb/regs.h"
#include "gdb/mem.h"
#include "gdb/watchpoints.h"
#include "gdb/stop_point.h"

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

    GDB_ResetWatchpoints();
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
            while(R_SUCCEEDED(svcGetProcessDebugEvent(&ctx->latestDebugEvent, ctx->debug)) &&
                 (ctx->latestDebugEvent.type != DBGEVENT_EXCEPTION || ctx->latestDebugEvent.exception.type != EXCEVENT_ATTACH_BREAK))
                svcContinueDebugEvent(ctx->debug, ctx->continueFlags);
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
    DebugEventInfo dummy;
    while(R_SUCCEEDED(svcGetProcessDebugEvent(&dummy, ctx->debug)));
    while(R_SUCCEEDED(svcContinueDebugEvent(ctx->debug, (DebugFlags)0)));
    RecursiveLock_Unlock(&ctx->lock);

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
    ctx->continueFlags = (DebugFlags)(DBG_SIGNAL_FAULT_EXCEPTION_EVENTS | DBG_INHIBIT_USER_CPU_EXCEPTION_HANDLERS);
    ctx->pid = 0;
    ctx->currentThreadId = ctx->selectedThreadId = 0;

    ctx->catchThreadEvents = ctx->processExited = ctx->processEnded = false;
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
            return GDB_HandleVerboseCommand;

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

        case 'z':
        case 'Z':
            return GDB_HandleToggleStopPoint;

        default:
            return GDB_HandleUnsupported;
    }
}

int GDB_DoPacket(sock_ctx *socketCtx)
{
    int ret;
    GDBContext *ctx = (GDBContext *)socketCtx;

    RecursiveLock_Lock(&ctx->lock);
    GDBFlags oldFlags = ctx->flags;

    if(ctx->state == GDB_STATE_DISCONNECTED)
        return -1;

    int r = GDB_ReceivePacket(ctx);
    if(r == -1)
        ret = -1;
    else if(ctx->buffer[0] == '\x03')
    {
        GDB_HandleBreak(ctx);
        ret = 0;
    }
    else if(ctx->buffer[0] == '$')
    {
        GDBCommandHandler handler = GDB_GetCommandHandler(ctx->buffer[1]);
        if(handler != NULL)
        {
            ctx->commandData = ctx->buffer + 2;
            ret = handler(ctx);
        }
        else
            ret = GDB_HandleUnsupported(ctx); // We don't have a handler!
    }
    else
        ret = 0;

    RecursiveLock_Unlock(&ctx->lock);
    if(ctx->state == GDB_STATE_CLOSING)
    {
        if(ctx->flags & GDB_FLAG_TERMINATE_PROCESS)
        {
            svcTerminateDebugProcess(ctx->debug);
            ctx->processEnded = true;
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
