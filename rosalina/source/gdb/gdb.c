#include <3ds.h>
#include "gdb/client_ctx.h"
#include "draw.h"

void gdb_setup_client(struct gdb_client_ctx *ctx)
{
	ctx->flags |= GDB_FLAG_USED;
}

void gdb_destroy_client(struct gdb_client_ctx *ctx)
{
	ctx->flags &= ~GDB_FLAG_USED;
}

int gdb_do_packet(Handle socket, struct gdb_client_ctx *ctx)
{
	draw_string("Do packet or something.", 10, 10, COLOR_TITLE);
    draw_flushFramebuffer();
	return 0;
}