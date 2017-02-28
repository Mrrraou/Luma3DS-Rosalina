#pragma once
#include <3ds/types.h>
#include <poll.h>

#define MAX_CLIENTS 8

struct sock_server;

typedef int (*sock_accept_cb)(Handle sock, void **ctx);
typedef int (*sock_data_cb)(Handle sock, void *ctx);
typedef int (*sock_close_cb)(Handle sock, void *ctx);

typedef void* (*sock_alloc_func)(struct sock_server *serv, Handle sock);
typedef void (*sock_free_func)(struct sock_server *serv, void *ctx);

enum socket_type
{
	SOCK_NONE,
	SOCK_SERVER,
	SOCK_CLIENT
};

struct sock_ctx
{
	enum socket_type type;
	void *data;
};

struct sock_server
{
	// params
	u32 host;
	void *userdata;

	// poll stuff
	Handle sock;
	struct pollfd poll_fds[MAX_CLIENTS];
	struct sock_ctx ctxs[MAX_CLIENTS];
	nfds_t nfds;
	bool running;
	bool compact_needed;

	// callbacks
	sock_accept_cb accept_cb;
	sock_data_cb data_cb;
	sock_close_cb close_cb;

	sock_alloc_func alloc;
	sock_free_func free;
};

void server_bind(struct sock_server *serv, u16 port, void *opt_data);
void server_run(struct sock_server *serv);
void server_close(struct sock_server *serv, int i);