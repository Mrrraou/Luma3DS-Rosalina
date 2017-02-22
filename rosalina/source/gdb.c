#include <3ds.h>
#include "gdb_ctx.h"
#include "draw.h"

void* gdb_get_client(struct sock_server *serv, Handle sock);
void gdb_release_client(struct sock_server *serv, void *ctx);
int gdb_do_packet(Handle socket, void *ctx);


void* gdb_get_client(struct sock_server *serv, Handle sock)
{
	struct gdb_client_ctx *ctxs = (struct gdb_client_ctx *)serv->userdata;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!(ctxs[i].flags & GDB_FLAG_USED))
		{
			ctxs[i].flags |= GDB_FLAG_USED;
			return &ctxs[i];
		}
	}

	return NULL;
}

void gdb_release_client(struct sock_server *serv, void *c)
{
	struct gdb_client_ctx *ctx = (struct gdb_client_ctx *)c;
	ctx->flags &= ~GDB_FLAG_USED;
}

int gdb_do_packet(Handle socket, void *c)
{
	struct gdb_client_ctx *ctx = (struct gdb_client_ctx *)c;

	draw_string("Do packet or something.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();
    
	return -1;
}