#pragma once
#include "sock_util.h"

enum gdb_flags
{
	GDB_FLAG_USED  = 1,
};

struct gdb_client_ctx
{
	enum gdb_flags flags;
};

void* gdb_get_client(struct sock_server *serv, Handle sock);
void gdb_release_client(struct sock_server *serv, void *ctx);
int gdb_do_packet(Handle socket, void *ctx);
