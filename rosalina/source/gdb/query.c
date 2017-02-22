#include "gdb_ctx.h"
#include "draw.h"
#include "minisoc.h"
#include "memory.h"

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
	{"StartNoAckMode", gdb_start_noack, GDB_DIR_WRITE},
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

	draw_string(name_begin, 10, 20, COLOR_WHITE);
	if(direction == GDB_DIR_READ)
		draw_string("read", 10, 30, COLOR_WHITE);
	else
		draw_string("write", 10, 30, COLOR_WHITE);
	for(int i = 0; gdb_query_handlers[i].name != NULL; i++)
	{
		if(strncmp(gdb_query_handlers[i].name, name_begin, strlen(gdb_query_handlers[i].name)) == 0 && gdb_query_handlers[i].direction == direction)
		{
			return gdb_query_handlers[i].f(sock, c, query_data);
		}
	}

	draw_string("No handler found!!!", 10, 30, COLOR_WHITE);
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

int gdb_handle_supported(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
	(void)c; // unused
	(void)buffer; // unused

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

	char *supp = "PacketSize=400;qXfer:features:read+;multiprocess+;QStartNoAckMode+";
	return gdb_send_packet(sock, supp, strlen(supp));
}

int gdb_start_noack(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
	(void)buffer; // unused
	c->state = GDB_STATE_NOACK_SENT;
	return gdb_reply_ok(sock);	
}

