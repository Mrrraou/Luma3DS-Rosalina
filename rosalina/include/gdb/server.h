#pragma once

#include "gdb.h"
#include "minisoc.h"
#include "sock_util.h"

extern sock_server gdbServer;
extern GDBContext  gdbCtxs[MAX_DEBUG];

int GDB_AcceptClient(sock_ctx *socketCtx);
int GDB_CloseClient(sock_ctx *socketCtx);
sock_ctx *GDB_GetClient(sock_server *socketSrv);
void GDB_ReleaseClient(sock_server *socketSrv, sock_ctx *socketCtx);
int GDB_DoPacket(sock_ctx *socketCtx);
