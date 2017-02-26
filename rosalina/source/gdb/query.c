#include "gdb_ctx.h"
#include "minisoc.h"
#include "memory.h"
#include "macros.h"

enum gdb_query_dir
{
	GDB_DIR_READ,
	GDB_DIR_WRITE
};

struct gdb_query_handler
{
	const char *name;
	gdb_command_handler f;
	enum gdb_query_dir direction;
};

struct gdb_query_handler gdb_query_handlers[] = 
{
	{"Supported", gdb_handle_supported, GDB_DIR_READ},
	{"Xfer", gdb_handle_xfer, GDB_DIR_READ},
	{"StartNoAckMode", gdb_start_noack, GDB_DIR_WRITE},
	{"Attached", gdb_handle_attached, GDB_DIR_READ},
	{"TStatus", gdb_tracepoint_status, GDB_DIR_READ},
	{"fThreadInfo", gdb_thread_info, GDB_DIR_READ},
	{NULL, NULL, 0}
};

int gdb_handle_query(Handle sock, struct gdb_client_ctx *c, char *buffer, enum gdb_query_dir direction)
{
	char *name_begin = buffer + 1; // remove leading 'q'/'Q'

	char *name_end = (char*)strchr(buffer, ':');
	char *query_data = NULL;

	if(name_end != NULL)
	{
		*name_end = 0;
		query_data = name_end + 1;
	}

	for(int i = 0; gdb_query_handlers[i].name != NULL; i++)
	{
		if(strncmp(gdb_query_handlers[i].name, name_begin, strlen(gdb_query_handlers[i].name)) == 0 && gdb_query_handlers[i].direction == direction)
		{
			return gdb_query_handlers[i].f(sock, c, query_data);
		}
	}

	return -1; // No handler found!
}

int gdb_handle_read_query(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
	return gdb_handle_query(sock, c, buffer, GDB_DIR_READ);
}

int gdb_handle_write_query(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
	return gdb_handle_query(sock, c, buffer, GDB_DIR_WRITE);
}

int gdb_handle_supported(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	/*char *item_start = buffer;
	int i = 0;

	while(true)
	{
		buffer = (char*)strchr(item_start, ';');
		if(buffer == NULL) break;

		*buffer = 0;
		draw_string(item_start, 10, 30+(i++*10), COLOR_WHITE);
		item_start = buffer + 1;
	}

	draw_string(item_start, 10, 30+(i++*10), COLOR_WHITE);*/

	char *supp = "PacketSize=400;qXfer:features:read+;QStartNoAckMode+";
	return gdb_send_packet(sock, supp, strlen(supp));
}

int gdb_start_noack(Handle sock, struct gdb_client_ctx *c, char *buffer UNUSED)
{
	c->state = GDB_STATE_NOACK_SENT;
	return gdb_reply_ok(sock);	
}

// TODO: stub
int gdb_handle_attached(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	return gdb_send_packet(sock, "1", 1);
}

// TODO: stub
int gdb_tracepoint_status(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	return gdb_send_packet(sock, "T0", 2);
}

// TODO: stub
int gdb_thread_info(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	return gdb_send_packet(sock, "l", 1);
}
