#include <3ds.h>
#include "minisoc.h"
#include "gdb_ctx.h"
#include "draw.h"
#include "memory.h"

gdb_command_handler gdb_command_handlers[GDB_NUM_COMMANDS] = 
{
	gdb_handle_unk,   // GDB_COMMAND_UNK
	gdb_handle_read_query, // GDB_COMMAND_QUERY_READ
	gdb_handle_write_query, // GDB_COMMAND_QUERY_WRITE
	gdb_handle_long // GDB_COMMAND_LONG 
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

		default:
			return GDB_COMMAND_UNK;
		break;
	}
}

void* gdb_get_client(struct sock_server *serv, Handle sock)
{
	(void)sock; // unused

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

void gdb_release_client(struct sock_server *serv, void *c)
{
	(void)serv; // unused

	struct gdb_client_ctx *ctx = (struct gdb_client_ctx *)c;
	ctx->flags &= ~GDB_FLAG_USED;
}

#define GDB_BUF_LEN 512
char gdb_buffer[GDB_BUF_LEN];

int gdb_send_packet(Handle socket, char *pkt_data, size_t len)
{
	gdb_buffer[0] = '$';
	memcpy(gdb_buffer + 1, pkt_data, len);

	uint8_t cksum = 0;
	for(size_t i = 0; i < len; i++)
	{
		cksum += (uint8_t)pkt_data[i];
	}

	char *cksum_loc = gdb_buffer + len + 1;
	*cksum_loc++ = '#';

	hexItoa(cksum, cksum_loc, 2, false);

	return soc_send(socket, gdb_buffer, len+4, 0);
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
						return;
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

			if(r == -1) { return -1; }
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

int gdb_handle_unk(Handle sock, struct gdb_client_ctx *c, char *buffer)
{
	(void)c; // unused
	(void)buffer; // unused

	return gdb_reply_empty(sock);
}