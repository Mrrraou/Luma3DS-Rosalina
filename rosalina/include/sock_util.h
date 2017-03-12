#pragma once
#include <3ds/types.h>
#include <poll.h>

#define MAX_PORTS 3
#define MAX_CTXS  (2 * MAX_PORTS)

struct sock_server;
struct sock_ctx;

typedef int (*sock_accept_cb)(struct sock_ctx *client_ctx);
typedef int (*sock_data_cb)(struct sock_ctx *client_ctx);
typedef int (*sock_close_cb)(struct sock_ctx *client_ctx);

typedef struct sock_ctx* (*sock_alloc_func)(struct sock_server *serv);
typedef void (*sock_free_func)(struct sock_server *serv, struct sock_ctx *ctx);

typedef enum socket_type
{
    SOCK_NONE,
    SOCK_SERVER,
    SOCK_CLIENT
} socket_type;

typedef struct sock_ctx
{
    enum socket_type type;
    Handle sock;
    struct sock_ctx *serv;
    int n;
    int i;
} sock_ctx;

typedef struct sock_server
{
    // params
    u32 host;
    void *userdata;
    int clients_per_server;

    // poll stuff
    struct pollfd poll_fds[MAX_CTXS];
    struct sock_ctx serv_ctxs[MAX_PORTS];
    struct sock_ctx *ctx_ptrs[MAX_CTXS];

    nfds_t nfds;
    bool running;
    bool compact_needed;

    // callbacks
    sock_accept_cb accept_cb;
    sock_data_cb data_cb;
    sock_close_cb close_cb;

    sock_alloc_func alloc;
    sock_free_func free;
} sock_server;

void server_init(struct sock_server *serv);
void server_bind(struct sock_server *serv, u16 port);
void server_run(struct sock_server *serv);
void server_close(struct sock_server *serv, struct sock_ctx *ctx);
struct sock_ctx *server_alloc_server_ctx(struct sock_server *serv);
