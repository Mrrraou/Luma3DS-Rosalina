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

struct sock_server
{
	// params
	u32 host;
	u16 port;
	void *userdata;

	// poll stuff
	Handle sock;
	struct pollfd poll_fds[MAX_CLIENTS];
	void *client_ctxs[MAX_CLIENTS];
	nfds_t nfds;
	bool running;

	// callbacks
	sock_accept_cb accept_cb;
	sock_data_cb data_cb;
	sock_close_cb close_cb;

	sock_alloc_func alloc;
	sock_free_func free;
};

void server_bind(struct sock_server *serv);
void server_run(struct sock_server *serv);