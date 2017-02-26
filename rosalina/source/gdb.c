#include <3ds.h>
#include "minisoc.h"
#include "gdb_ctx.h"
#include "draw.h"
#include "memory.h"
#include "macros.h"

char gdb_buffer[GDB_BUF_LEN];

gdb_command_handler gdb_command_handlers[GDB_NUM_COMMANDS] = 
{
	gdb_handle_unk,   // GDB_COMMAND_UNK
	gdb_handle_read_query, // GDB_COMMAND_QUERY_READ
	gdb_handle_write_query, // GDB_COMMAND_QUERY_WRITE
	gdb_handle_long, // GDB_COMMAND_LONG
	gdb_handle_stopped, // GDB_COMMAND_STOP_REASON
	gdb_handle_read_regs, // GDB_COMMAND_READ_REGS
	gdb_handle_read_mem // GDB_COMMAND_READ_MEM
};

enum gdb_command gdb_get_cmd(char c)
{
	switch(c)
	{
		case 'q':
			return GDB_COMMAND_QUERY_READ;
		break;

		case 'Q':
			return GDB_COMMAND_QUERY_WRITE;
		break;

		case 'v':
			return GDB_COMMAND_LONG;
		break;

		case '?':
			return GDB_COMMAND_STOP_REASON;
		break;

		case 'g':
			return GDB_COMMAND_READ_REGS;
		break;

		case 'm':
			return GDB_COMMAND_READ_MEM;
		break;


		default:
			return GDB_COMMAND_UNK;
		break;
	}
}

void* gdb_get_client(struct sock_server *serv, Handle sock UNUSED)
{
	struct gdb_client_ctx *ctxs = (struct gdb_client_ctx *)serv->userdata;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!(ctxs[i].flags & GDB_FLAG_USED))
		{
			ctxs[i].flags |= GDB_FLAG_USED;
			ctxs[i].state = GDB_STATE_CONNECTED;

			return &ctxs[i];
		}
	}

	return NULL;
}

void gdb_release_client(struct sock_server *serv UNUSED, void *c)
{
	struct gdb_client_ctx *ctx = (struct gdb_client_ctx *)c;
	ctx->flags &= ~GDB_FLAG_USED;
}

int gdb_do_packet(Handle socket, void *c)
{
	struct gdb_client_ctx *ctx = (struct gdb_client_ctx *)c;

	switch(ctx->state)
	{
		// Both of these require a '+'.
		case GDB_STATE_CONNECTED:
		case GDB_STATE_NOACK_SENT:
		{
			int r = soc_recv(socket, gdb_buffer, 1, 0);
			if(r != 1)
			{
				return -1;
			}
			else
			{
				if(gdb_buffer[0] != '+')
				{
					return -1;
				}
				else
				{
					if(ctx->state == GDB_STATE_NOACK_SENT)
					{
						ctx->state = GDB_STATE_NOACK;
						return 0;
					}
				}
			}

			r = soc_send(socket, "+", 1, 0); // Yes. :(
			if(r == -1) { return -1; }
		}

		// lack of break is intentional
		case GDB_STATE_NOACK:
		{
			char cksum[3];
			cksum[2] = 0;

			int r = soc_recv_until(socket, gdb_buffer, GDB_BUF_LEN, "#", 1);
			soc_recv(socket, cksum, 2, 0);

			if(r == 0 || r == -1) { return -1; } // Bubbling -1 up to server will close the connection.
			else
			{
				gdb_buffer[r-1] = 0; // replace trailing '#' with 0

				enum gdb_command cmd = gdb_get_cmd(gdb_buffer[1]);
				if(gdb_command_handlers[cmd] != NULL)
				{
					int res = gdb_command_handlers[cmd](socket, ctx, gdb_buffer + 1);
					if(res == -1)
					{
						char s[] = "  wtf";
						s[0] = gdb_buffer[1];
						draw_string(s, 10, 40, COLOR_WHITE);
						return gdb_reply_empty(socket); // Handler failed!
					}
					return res;
				}
				else
				{
					draw_string(gdb_buffer, 10, 30, COLOR_WHITE);
					draw_string("WTFF", 10, 50, COLOR_WHITE);
					return gdb_reply_empty(socket); // We don't have a handler!
				}
			}
		}
		break;
	}

	return -1;
}

int gdb_reply_empty(Handle sock)
{
	return soc_send(sock, "$#00", 4, 0);
}

int gdb_reply_ok(Handle sock)
{
	return soc_send(sock, "$OK#9a", 6, 0);
}

int gdb_handle_unk(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	return gdb_reply_empty(sock);
}

// TODO: stub
int gdb_handle_stopped(Handle sock, struct gdb_client_ctx *c UNUSED, char *buffer UNUSED)
{
	return gdb_send_packet(sock, "S01", 3);
}