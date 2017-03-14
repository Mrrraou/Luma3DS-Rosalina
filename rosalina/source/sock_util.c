#include <sys/socket.h>
#include "memory.h"
#include "minisoc.h"
#include "sock_util.h"
#include "menu.h"

// soc's poll function is odd, and doesn't like -1 as fd.
// so this compacts everything together

void compact(struct sock_server *serv)
{
    int new_fds[MAX_CTXS];
    struct sock_ctx *new_ctxs[MAX_CTXS];
    nfds_t n = 0;

    for(nfds_t i = 0; i < serv->nfds; i++)
    {
        if(serv->poll_fds[i].fd != -1)
        {
            new_fds[n] = serv->poll_fds[i].fd;
            new_ctxs[n] = serv->ctx_ptrs[i];
            n++;
        }
    }

    for(nfds_t i = 0; i < n; i++)
    {
        serv->poll_fds[i].fd = new_fds[i];
        serv->ctx_ptrs[i] = new_ctxs[i];
        serv->ctx_ptrs[i]->i = i;
    }
    serv->nfds = n;
    serv->compact_needed = false;
}

void server_close_ctx(struct sock_server *serv, struct sock_ctx *ctx)
{
    serv->compact_needed = true;

    Handle sock = serv->poll_fds[ctx->i].fd;
    if(ctx->type == SOCK_CLIENT)
    {
        if(serv->close_cb != NULL) serv->close_cb(ctx);
    }

    socClose(sock);
    serv->poll_fds[ctx->i].fd = -1;
    serv->poll_fds[ctx->i].events = 0;
    serv->poll_fds[ctx->i].revents = 0;

    ctx->type = SOCK_NONE;

    serv->free(serv, ctx);

    ctx->serv->n--;
}

void server_init(struct sock_server *serv)
{
    memset_(serv, 0, sizeof(struct sock_server));

    for(int i = 0; i < MAX_PORTS; i++)
        serv->serv_ctxs[i].type = SOCK_NONE;

    for(int i = 0; i < MAX_CTXS; i++)
        serv->ctx_ptrs[i] = NULL;
}

void server_bind(struct sock_server *serv, u16 port)
{
    Handle server_sock;

    Result res = socSocket(&server_sock, AF_INET, SOCK_STREAM, 0);
    while(R_FAILED(res))
    {
        svcSleepThread(100000000LL);

        res = socSocket(&server_sock, AF_INET, SOCK_STREAM, 0);
    }

    if(R_SUCCEEDED(res))
    {
        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        saddr.sin_port = (port & 0xff00) >> 8 | (port & 0xff) << 8;
        saddr.sin_addr.s_addr = gethostid();

        res = socBind(server_sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in));

        if(R_SUCCEEDED(res))
        {
            res = socListen(server_sock, 2);
            if(R_SUCCEEDED(res))
            {
                int idx = serv->nfds;
                serv->nfds++;
                serv->poll_fds[idx].fd = server_sock;
                serv->poll_fds[idx].events = POLLIN | POLLHUP;

                struct sock_ctx *new_ctx = server_alloc_server_ctx(serv);
                new_ctx->type = SOCK_SERVER;
                new_ctx->sock = server_sock;
                new_ctx->n = 0;
                new_ctx->i = idx;
                serv->ctx_ptrs[idx] = new_ctx;
            }
        }
    }
}

struct sock_ctx *server_alloc_server_ctx(struct sock_server *serv)
{
    for(int i = 0; i < MAX_PORTS; i++)
    {
        if(serv->serv_ctxs[i].type == SOCK_NONE)
            return &serv->serv_ctxs[i];
    }

    return NULL;
}

void server_run(struct sock_server *serv)
{
    struct pollfd *fds = serv->poll_fds;
    serv->running = true;

    while(serv->running && !terminationRequest)
    {
        if(serv->nfds == 0)
        {
            svcSleepThread(50 * 1000 * 1000);
            continue;
        }

        int res = socPoll(fds, serv->nfds, 50); // 50ms
        if(res == 0) continue; // timeout reached, no activity.

        for(unsigned int i = 0; i < serv->nfds; i++)
        {
            struct sock_ctx *curr_ctx = serv->ctx_ptrs[i];

            if((fds[i].revents & POLLIN) == POLLIN)
            {
                if(curr_ctx->type == SOCK_SERVER) // Listening socket?
                {
                    Handle client_sock = 0;
                    res = socAccept(fds[i].fd, &client_sock, NULL, 0);

                    if(curr_ctx->n == serv->clients_per_server || serv->nfds == MAX_CTXS)
                        socClose(client_sock);

                    else
                    {
                        fds[serv->nfds].fd = client_sock;
                        fds[serv->nfds].events = POLLIN | POLLHUP;

                        int new_idx = serv->nfds;
                        serv->nfds++;
                        curr_ctx->n++;

                        struct sock_ctx *new_ctx = serv->alloc(serv);
                        new_ctx->type = SOCK_CLIENT;
                        new_ctx->sock = client_sock;
                        new_ctx->serv = curr_ctx;
                        new_ctx->i = new_idx;
                        new_ctx->n = 0;

                        serv->ctx_ptrs[new_idx] = new_ctx;

                        serv->accept_cb(new_ctx);
                    }
                }
                else
                {
                    if(serv->data_cb(curr_ctx) == -1)
                        server_close_ctx(serv, curr_ctx);
                }
            }
            else if(fds[i].revents & POLLHUP || fds[i].revents & POLLERR) // For some reason, this never gets hit?
                server_close_ctx(serv, curr_ctx);
        }

        if(serv->compact_needed)
            compact(serv);
    }

    // Clean up.
    for(unsigned int i = 0; i < serv->nfds; i++)
    {
        if(fds[i].fd != -1)
            socClose(fds[i].fd);
    }

    serv->running = false;
}

void server_stop(struct sock_server *serv)
{
    for(nfds_t i = 0; i < serv->nfds; i++)
        server_close_ctx(serv, serv->ctx_ptrs[i]);
    compact(serv);

    serv->running = false;
}
