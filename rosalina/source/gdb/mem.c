#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"

// TODO: stub
int gdb_handle_read_mem(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	/*const char *addr_start = buffer;
	char *addr_end = (char*)strchr(addr_start, ',');
	if(addr_end == NULL) return -1;

	*addr_end = 0;
	const char *len_start = addr_end+1;*/

	char s[] = "E01";
	return gdb_send_packet(sock, s, sizeof(s));
}