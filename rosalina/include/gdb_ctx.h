#pragma once
#include "sock_util.h"

#define MAX_DEBUG 3
#define GDB_BUF_LEN 512
extern char gdb_buffer[GDB_BUF_LEN];

enum gdb_flags
{
	GDB_FLAG_USED  = 1,
};

enum gdb_state
{
	GDB_STATE_CONNECTED,
	GDB_STATE_NOACK_SENT,
	GDB_STATE_NOACK,
};

enum gdb_command
{
	GDB_COMMAND_UNK,
	GDB_COMMAND_QUERY_READ,
	GDB_COMMAND_QUERY_WRITE,
	GDB_COMMAND_LONG,
	GDB_COMMAND_STOP_REASON,
	GDB_COMMAND_READ_REGS,
	GDB_COMMAND_READ_MEM,
	GDB_NUM_COMMANDS
};

struct gdb_client_ctx;

struct gdb_server_ctx
{
	enum gdb_flags flags;
	Handle debug;
	struct sock_ctx *client;
	struct gdb_client_ctx *client_gdb_ctx;
};

struct gdb_client_ctx
{
	enum gdb_flags flags;
	enum gdb_state state;
	struct gdb_server_ctx *proc;
};

Result debugger_attach(struct sock_server *serv, u32 pid);

int gdb_accept_client(struct sock_ctx *client_ctx);
void* gdb_get_client(struct sock_server *serv, struct sock_ctx *client_ctx);
void gdb_release_client(struct sock_server *serv, void *c);
int gdb_do_packet(struct sock_ctx *ctx);

int gdb_send_packet(Handle socket, const char *pkt_data, size_t len);
int gdb_send_packet_hex(Handle socket, const char *pkt_data, size_t len);
int gdb_send_packet_prefix(Handle socket, const char *prefix, size_t prefix_len, const char *pkt_data, size_t len);

typedef int (*gdb_command_handler)(Handle sock, struct gdb_client_ctx *c, char *buffer);
extern gdb_command_handler gdb_command_handlers[GDB_NUM_COMMANDS];

enum gdb_command gdb_get_cmd(char c);
int gdb_reply_empty(Handle sock);
int gdb_reply_ok(Handle sock);

int gdb_handle_unk(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_read_query(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_write_query(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_long(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_stopped(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_read_regs(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_read_mem(Handle sock, struct gdb_client_ctx *c, char *buffer);

// Queries
int gdb_handle_supported(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_start_noack(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_xfer(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_handle_attached(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_tracepoint_status(Handle sock, struct gdb_client_ctx *c, char *buffer);
int gdb_thread_info(Handle sock, struct gdb_client_ctx *c, char *buffer);