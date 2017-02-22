#include "gdb_ctx.h"
#include "draw.h"
#include "minisoc.h"
#include "memory.h"

struct gdb_long_handler
{
	const char *name;
	gdb_command_handler f;
};

struct gdb_long_handler gdb_long_handlers[] = 
{
	{"MustReplyEmpty", gdb_handle_unk}, // unknown packets are replied to with an empty packet..
	{NULL, NULL}
};

int gdb_handle_long(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
	char *name_begin = buffer + 1; // remove leading 'q'

	char *name_end = (char*)strchr(buffer, ';');
	char *v_data = NULL;

	if(name_end != NULL)
	{
		*name_end = 0;
		v_data = name_end+1;
	}

	//draw_string(buffer, 10, 20, COLOR_WHITE);

	for(int i = 0; gdb_long_handlers[i].name != NULL; i++)
	{
		if(strncmp(gdb_long_handlers[i].name, name_begin, strlen(gdb_long_handlers[i].name)) == 0)
		{
			return gdb_long_handlers[i].f(sock, c, v_data);
		}
	}

	return -1; // No handler found!
}