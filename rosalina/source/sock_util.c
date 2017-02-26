#include <sys/socket.h>
#include "memory.h"
#include "minisoc.h"
#include "sock_util.h"
#include "menu.h"

// soc's poll function is odd, and doesn't like -1 as fd.
// so this compacts everything together

void compact(struct sock_server *serv)
{

    int new_fds[MAX_CLIENTS];
    void *new_ctxs[MAX_CLIENTS];
    nfds_t n = 0;

    for(nfds_t i = 0; i < serv->nfds; i++)
    {
        if(serv->poll_fds[i].fd != -1)
        {
            new_fds[n] = serv->poll_fds[i].fd;
            new_ctxs[n] = serv->client_ctxs[i];
            n++;
        }
    }

    for(nfds_t i = 0; i < n; i++)
    {
        serv->poll_fds[i].fd = new_fds[i];
        serv->client_ctxs[i] = new_ctxs[i];
    }
    serv->nfds = n;
}

void close_then_compact(struct sock_server *serv, int i)
{
    Handle client_sock = serv->poll_fds[i].fd;
    if(serv->close_cb != NULL) serv->close_cb(client_sock, serv->client_ctxs[i]);

    socClose(client_sock);
    serv->poll_fds[i].fd = -1;
    serv->poll_fds[i].events = 0;
    serv->poll_fds[i].revents = 0;

    if(serv->client_ctxs[i] != NULL)
    {
        if(serv->free != NULL) serv->free(serv, serv->client_ctxs[i]);
        serv->client_ctxs[i] = NULL;
    }

    compact(serv);
}

void server_bind(struct sock_server *serv)
{
    Result res = socSocket(&serv->sock, AF_INET, SOCK_STREAM, 0);
    while(R_FAILED(res))
    {
        svcSleepThread(100000000LL);

        res = socSocket(&serv->sock, AF_INET, SOCK_STREAM, 0);
    }

    if(R_SUCCEEDED(res))
    {
        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        saddr.sin_port = 0xa00f; // 4000, byteswapped
        saddr.sin_addr.s_addr = gethostid();

        res = socBind(serv->sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in));

        if(R_SUCCEEDED(res))
        {
            res = socListen(serv->sock, 2);
            if(R_SUCCEEDED(res))
            {
                serv->nfds = 1;
                serv->poll_fds[0].fd = serv->sock;
                serv->poll_fds[0].events = POLLIN | POLLHUP;
            }
        }
    }
}

void server_run(struct sock_server *serv)
{
    struct pollfd *fds = serv->poll_fds;

    while(serv->running && !terminationRequest)
    {
        int res = socPoll(fds, serv->nfds, 50); // 50ms
        if(res == 0) continue; // timeout reached, no activity.

        for(unsigned int i = 0; i < serv->nfds; i++)
        {
            if((fds[i].revents & POLLIN) == POLLIN)
            {
                if(i == 0) // Listening socket?
                {
                    Handle client_sock = 0;
                    res = socAccept(serv->sock, &client_sock, NULL, 0);
                    
                    if(serv->nfds == MAX_CLIENTS)
                    {
                        socClose(client_sock);
                    }
                    else
                    {
                        fds[serv->nfds].fd = client_sock;
                        fds[serv->nfds].events = POLLIN | POLLHUP;
                        serv->nfds++;

                        if(serv->alloc != NULL)
                        {
                            void *new_ctx = serv->alloc(serv, client_sock);
                            if(new_ctx == NULL)
                            {
                                close_then_compact(serv, serv->nfds-1);
                            }
                            else
                            {
                                serv->client_ctxs[serv->nfds-1] = new_ctx;
                            }
                        }
                    }
                }
                else
                {
                    if(serv->data_cb != NULL)
                    {
                        if(serv->data_cb(fds[i].fd, serv->client_ctxs[i]) == -1)
                        {
                            close_then_compact(serv, i);
                        }
                    }
                }
            }
            else if(fds[i].revents & POLLHUP || fds[i].revents & POLLERR) // For some reason, this never gets hit?
            {
                close_then_compact(serv, i);
            }
        }
    }

    // Clean up.
    for(unsigned int i = 0; i < serv->nfds; i++)
    {
        socClose(fds[i].fd);
    }
}

#define PEEK_SIZE 512
char peek_buffer[PEEK_SIZE];

int soc_recv_until(Handle fd, char *buf, size_t buf_len, char *sig, size_t sig_len)
{
    int r = soc_recvfrom(fd, peek_buffer, PEEK_SIZE, MSG_PEEK, NULL, 0);
    if(r == 0 || r == -1)
    {
        return r;
    }

    char *ptr = (char*) memsearch((u8*)peek_buffer, sig, PEEK_SIZE, sig_len);
    if(ptr == NULL)
    {
        return -1; // Line too long!
    }
    size_t len = ptr - peek_buffer;
    len += sig_len;

    if(len > buf_len)
    {
        return -1;
    }
    memset_(peek_buffer, 0, len);

    r = soc_recvfrom(fd, buf, len, 0, NULL, 0);
    return r;
}