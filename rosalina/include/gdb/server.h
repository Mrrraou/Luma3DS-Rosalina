#pragma once

#include "gdb.h"

typedef struct GDBServer
{
    sock_server super;
    s32 referenceCount;
    Handle statusUpdated;
    GDBContext ctxs[MAX_DEBUG];
} GDBServer;

Result GDB_InitializeServer(GDBServer *server);
void GDB_FinalizeServer(GDBServer *server);

void GDB_IncrementServerReferenceCount(GDBServer *server);
void GDB_DecrementServerReferenceCount(GDBServer *server);

void GDB_RunServer(GDBServer *server);

int GDB_AcceptClient(GDBContext *ctx);
int GDB_CloseClient(GDBContext *ctx);
GDBContext *GDB_GetClient(GDBServer *server);
void GDB_ReleaseClient(GDBServer *server, GDBContext *ctx);
int GDB_DoPacket(GDBContext *ctx);
