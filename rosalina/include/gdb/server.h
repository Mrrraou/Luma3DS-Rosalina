#pragma once

#include "gdb.h"
#include "minisoc.h"
#include "sock_util.h"

typedef struct GDBServer
{
    sock_server super;
    Handle statusUpdated;
    GDBContext ctxs[MAX_DEBUG];
} GDBServer;

void GDB_InitializeServer(GDBServer *server);
void GDB_FinalizeServer(GDBServer *server);
void GDB_RunServer(GDBServer *server);
void GDB_StopServer(GDBServer *server);

int GDB_AcceptClient(sock_ctx *socketCtx);
int GDB_CloseClient(sock_ctx *socketCtx);
sock_ctx *GDB_GetClient(sock_server *socketSrv);
void GDB_ReleaseClient(sock_server *socketSrv, sock_ctx *socketCtx);
int GDB_DoPacket(sock_ctx *socketCtx);
