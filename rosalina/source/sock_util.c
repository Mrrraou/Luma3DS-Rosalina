#include <sys/socket.h>
#include <3ds/result.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include "memory.h"
#include "minisoc.h"
#include "sock_util.h"

extern Handle terminationRequestEvent;
extern bool terminationRequest;

static u32 soc_block_addr;
static size_t soc_block_size;

// soc's poll function is odd, and doesn't like -1 as fd.
// so this compacts everything together

static void compact(struct sock_server *serv)
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

static struct sock_ctx *server_alloc_server_ctx(struct sock_server *serv)
{
    for(int i = 0; i < MAX_PORTS; i++)
    {
        if(serv->serv_ctxs[i].type == SOCK_NONE)
            return &serv->serv_ctxs[i];
    }

    return NULL;
}

static void server_close_ctx(struct sock_server *serv, struct sock_ctx *ctx)
{
    serv->compact_needed = true;

    Handle sock = serv->poll_fds[ctx->i].fd;
    if(ctx->type == SOCK_CLIENT)
    {
        serv->close_cb(ctx);
        serv->free(serv, ctx);
        ctx->serv->n--;
    }

    socClose(sock);
    ctx->should_close = false;

    serv->poll_fds[ctx->i].fd = -1;
    serv->poll_fds[ctx->i].events = 0;
    serv->poll_fds[ctx->i].revents = 0;

    ctx->type = SOCK_NONE;
}

static s32 miniSocRefCount = 0;
Result server_init(struct sock_server *serv)
{
    Result ret = 0;

    if(AtomicPostIncrement(&miniSocRefCount) == 0)
    {
        soc_block_addr = 0x08000000;
        soc_block_size = 0x30000;

        u32 tmp = 0;
        Result ret = svcControlMemory(&tmp, soc_block_addr, 0, soc_block_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);
        if(ret != 0)
        {
            AtomicDecrement(&miniSocRefCount);
            return ret;
        }

        soc_block_addr = tmp;
        ret = miniSocInit((u32 *)soc_block_addr, soc_block_size);

        if(ret != 0)
        {
            svcControlMemory(&tmp, soc_block_addr, soc_block_addr, soc_block_size, MEMOP_FREE, MEMPERM_DONTCARE);
            AtomicDecrement(&miniSocRefCount);
            return ret;
        }
    }

    memset_(serv, 0, sizeof(struct sock_server));

    for(int i = 0; i < MAX_PORTS; i++)
        serv->serv_ctxs[i].type = SOCK_NONE;

    for(int i = 0; i < MAX_CTXS; i++)
        serv->ctx_ptrs[i] = NULL;

    ret = svcCreateEvent(&serv->started_event, RESET_STICKY);
    if(R_FAILED(ret))
        return ret;
    return svcCreateEvent(&serv->shall_terminate_event, RESET_STICKY);
}

void server_bind(struct sock_server *serv, u16 port)
{
    int server_sockfd;
    Handle handles[2] = { terminationRequestEvent, serv->shall_terminate_event };
    s32 idx = -1;
    server_sockfd = socSocket(AF_INET, SOCK_STREAM, 0);
    int res;

    while(server_sockfd == -1)
    {
        if(svcWaitSynchronizationN(&idx, handles, 2, false, 100 * 1000 * 1000LL) == 0)
            return;

        server_sockfd = socSocket(AF_INET, SOCK_STREAM, 0);
    }

    if(server_sockfd != -1)
    {
        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        saddr.sin_port = (port & 0xff00) >> 8 | (port & 0xff) << 8;
        saddr.sin_addr.s_addr = gethostid();

        res = socBind(server_sockfd, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in));

        if(res == 0)
        {
            res = socListen(server_sockfd, 2);
            if(res == 0)
            {
                int idx = serv->nfds;
                serv->nfds++;
                serv->poll_fds[idx].fd = server_sockfd;
                serv->poll_fds[idx].events = POLLIN | POLLHUP | POLLNVAL;

                struct sock_ctx *new_ctx = server_alloc_server_ctx(serv);
                memcpy(&new_ctx->addr_in, &saddr, sizeof(struct sockaddr_in));
                new_ctx->type = SOCK_SERVER;
                new_ctx->sockfd = server_sockfd;
                new_ctx->n = 0;
                new_ctx->i = idx;
                serv->ctx_ptrs[idx] = new_ctx;
            }
        }
    }
}

void server_run(struct sock_server *serv)
{
    struct pollfd *fds = serv->poll_fds;
    Handle handles[2] = { terminationRequestEvent, serv->shall_terminate_event };


    serv->running = true;
    svcSignalEvent(serv->started_event);

    while(serv->running && !terminationRequest)
    {
        s32 idx = -1;
        if(svcWaitSynchronizationN(&idx, handles, 2, false, 0LL) == 0)
            goto abort_connections;

        if(serv->nfds == 0)
        {
            if(svcWaitSynchronizationN(&idx, handles, 2, false, 12 * 1000 * 1000LL) == 0)
                goto abort_connections;
            else
                continue;
        }

        socPoll(fds, serv->nfds, 50);

        for(unsigned int i = 0; i < serv->nfds; i++)
        {
            struct sock_ctx *curr_ctx = serv->ctx_ptrs[i];

            if((fds[i].revents & POLLHUP) || (fds[i].revents & POLLERR) || (fds[i].revents & POLLNVAL) || curr_ctx->should_close)
                server_close_ctx(serv, curr_ctx);

            else if((fds[i].revents & POLLIN) == POLLIN)
            {
                if(curr_ctx->type == SOCK_SERVER) // Listening socket?
                {
                    struct sockaddr_in saddr;
                    socklen_t len = sizeof(struct sockaddr_in);
                    int client_sockfd = socAccept(fds[i].fd, (struct sockaddr *)&saddr, &len);

                    if(svcWaitSynchronizationN(&idx, handles, 2, false, 0LL) == 0)
                        goto abort_connections;

                    if(client_sockfd < 0 || curr_ctx->n == serv->clients_per_server || serv->nfds == MAX_CTXS)
                        socClose(client_sockfd);

                    else
                    {
                        struct sock_ctx *new_ctx = serv->alloc(serv);
                        if(new_ctx == NULL)
                            socClose(client_sockfd);
                        else
                        {
                            fds[serv->nfds].fd = client_sockfd;
                            fds[serv->nfds].events = POLLIN | POLLHUP | POLLNVAL;

                            int new_idx = serv->nfds;
                            serv->nfds++;
                            curr_ctx->n++;

                            new_ctx->type = SOCK_CLIENT;
                            new_ctx->sockfd = client_sockfd;
                            new_ctx->serv = curr_ctx;
                            new_ctx->i = new_idx;
                            new_ctx->n = 0;

                            serv->ctx_ptrs[new_idx] = new_ctx;

                            if(serv->accept_cb(new_ctx) == -1)
                                server_close_ctx(serv, new_ctx);

                            memcpy(&new_ctx->addr_in, &saddr, sizeof(struct sockaddr_in));
                        }
                    }
                }
                else
                {
                    if(serv->data_cb(curr_ctx) == -1)
                        server_close_ctx(serv, curr_ctx);
                }
            }
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
    svcClearEvent(serv->started_event);
    return;

abort_connections:
    for(unsigned int i = 0; i < serv->nfds; i++)
    {
        struct linger linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;

        socSetsockopt(fds[i].fd, SOL_SOCKET, SO_LINGER, &linger, sizeof(struct linger));
        socClose(fds[i].fd);
    }

    serv->running = false;
    svcClearEvent(serv->started_event);
}

void server_finalize(struct sock_server *serv)
{
    for(nfds_t i = 0; i < MAX_CTXS; i++)
    {
        if(serv->ctx_ptrs[i] != NULL)
            server_close_ctx(serv, serv->ctx_ptrs[i]);
    }

    if(AtomicDecrement(&miniSocRefCount) == 0)
    {
        u32 tmp;
        miniSocExit();
        svcControlMemory(&tmp, soc_block_addr, soc_block_addr, soc_block_size, MEMOP_FREE, MEMPERM_DONTCARE);
    }

    svcClearEvent(serv->shall_terminate_event);
    svcCloseHandle(serv->shall_terminate_event);
    svcClearEvent(serv->started_event);
    svcCloseHandle(serv->started_event);
}
