#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"

// TODO: stub
int gdb_handle_read_regs(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	u32 regs[17];
	memset_(regs, 0, sizeof(regs));
	return gdb_send_packet_hex(sock, (const char *)regs, sizeof(regs));
}