#include <3ds.h>
#include "minisoc.h"
#include "gdb_ctx.h"
#include "memory.h"
#include "macros.h"
#include "menus/debugger.h"

char gdb_buffer[GDB_BUF_LEN + 4];

gdb_command_handler gdb_get_cmd_handler(char c)
{
	switch(c)
	{
		case 'q':
			return gdb_handle_read_query;

		case 'Q':
			return gdb_handle_write_query;

		case 'v':
			return gdb_handle_long;

		case '?':
			return gdb_handle_stopped;

		case 'g':
			return gdb_handle_read_regs;

		case 'G':
			return gdb_handle_write_regs;

		case 'p':
			return gdb_handle_read_reg;

		case 'P':
			return gdb_handle_write_reg;

		case 'm':
			return gdb_handle_read_mem;

        case 'H':
            return gdb_handle_set_thread_id;

		case 'c':
		case 'C':
			return gdb_handle_continue;

		default:
			return gdb_handle_unk;
	}
}

int gdb_accept_client(struct sock_ctx *client_ctx)
{
	struct gdb_server_ctx *s_ctx = (struct gdb_server_ctx *)client_ctx->serv->data;
	struct gdb_client_ctx *c_ctx = (struct gdb_client_ctx *)client_ctx->data;

	c_ctx->proc = s_ctx;
	s_ctx->client = client_ctx;
	s_ctx->client_gdb_ctx = c_ctx;

	RecursiveLock_Init(&c_ctx->sock_lock);
	svcSignalEvent(s_ctx->clientAcceptedEvent);

	return 0;
}

int gdb_close_client(struct sock_ctx *client_ctx)
{
	struct gdb_server_ctx *s_ctx = (struct gdb_server_ctx *)client_ctx->serv->data;
	s_ctx->client = NULL;
	s_ctx->client_gdb_ctx = NULL;

	return 0;
}

void* gdb_get_client(struct sock_server *serv, struct sock_ctx *c UNUSED)
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

int gdb_do_packet(struct sock_ctx *c)
{
	int ret = -1;
	Handle socket = c->sock;
	struct gdb_client_ctx *ctx = (struct gdb_client_ctx *)c->data;

	RecursiveLock_Lock(&ctx->sock_lock);

	switch(ctx->state)
	{
		// Both of these require a '+'.
		case GDB_STATE_CONNECTED:
		case GDB_STATE_NOACK_SENT:
		{
			int r = soc_recv(socket, gdb_buffer, 1, 0);
			if(r != 1)
			{
				ret = -1;
				goto unlock;
			}
			else
			{
				if(gdb_buffer[0] != '+')
				{
					ret = -1;
					goto unlock;
				}
				else
				{
					if(ctx->state == GDB_STATE_NOACK_SENT)
					{
						ctx->state = GDB_STATE_NOACK;
						ret = 0;
						goto unlock;
					}
				}
			}

			r = soc_send(socket, "+", 1, 0); // Yes. :(
			if(r == -1) { ret = -1; goto unlock; }
		}

		// lack of break is intentional
		case GDB_STATE_NOACK:
		{
			char cksum[3];
			cksum[2] = 0;

			int r = soc_recv_until(socket, gdb_buffer, GDB_BUF_LEN, "#", 1);
			if(gdb_buffer[0] == '\x03')
			{
				gdb_handle_break(socket, ctx, NULL);
				goto unlock;
			}

			soc_recv(socket, cksum, 2, 0);

			if(r == 0 || r == -1) { ret = -1; goto unlock; } // Bubbling -1 up to server will close the connection.
			else
			{
				gdb_buffer[r-1] = 0; // replace trailing '#' with 0

				gdb_command_handler handler = gdb_get_cmd_handler(gdb_buffer[1]);
				if(handler != NULL)
				{
					int res = handler(socket, ctx, gdb_buffer + 1);
					if(res == -1)
					{
						ret = gdb_reply_empty(socket); // Handler failed!
						goto unlock;
					}
					ret = res;
					goto unlock;
				}
				else
				{
					ret = gdb_reply_empty(socket); // We don't have a handler!
					goto unlock;
				}
			}
		}
		break;
	}

	unlock:
	RecursiveLock_Unlock(&ctx->sock_lock);

	return ret;
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
