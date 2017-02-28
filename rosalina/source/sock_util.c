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
    struct sock_ctx new_ctxs[MAX_CLIENTS];
    nfds_t n = 0;

    for(nfds_t i = 0; i < serv->nfds; i++)
    {
        if(serv->poll_fds[i].fd != -1)
        {
            new_fds[n] = serv->poll_fds[i].fd;
            new_ctxs[n] = serv->ctxs[i];
            n++;
        }
    }

    for(nfds_t i = 0; i < n; i++)
    {
        serv->poll_fds[i].fd = new_fds[i];
        serv->ctxs[i] = new_ctxs[i];
    }
    serv->nfds = n;
    serv->compact_needed = false;
}

void server_close(struct sock_server *serv, int i)
{
    serv->compact_needed = true;

    Handle sock = serv->poll_fds[i].fd;
    if(serv->ctxs[i].type == SOCK_CLIENT)
    {
        if(serv->close_cb != NULL) serv->close_cb(sock, serv->ctxs[i].data);
    }

    socClose(sock);
    serv->poll_fds[i].fd = -1;
    serv->poll_fds[i].events = 0;
    serv->poll_fds[i].revents = 0;

    serv->ctxs[i].type = SOCK_NONE;
    if(serv->ctxs[i].data != NULL)
    {
        if(serv->free != NULL) serv->free(serv, serv->ctxs[i].data);
        serv->ctxs[i].data = NULL;
    }
}

void server_bind(struct sock_server *serv, u16 port, void *opt_data)
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
        saddr.sin_port = (port & 0xff00) >> 8 | (port & 0xff) << 8;
        saddr.sin_addr.s_addr = gethostid();

        res = socBind(serv->sock, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in));

        if(R_SUCCEEDED(res))
        {
            res = socListen(serv->sock, 2);
            if(R_SUCCEEDED(res))
            {
                int idx = serv->nfds;
                serv->nfds++;
                serv->poll_fds[idx].fd = serv->sock;
                serv->poll_fds[idx].events = POLLIN | POLLHUP;
                serv->ctxs[idx].type = SOCK_SERVER;
                serv->ctxs[idx].data = opt_data;
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
                if(serv->ctxs[i].type == SOCK_SERVER) // Listening socket?
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

                        int idx = serv->nfds;
                        serv->nfds++;

                        if(serv->alloc != NULL)
                        {
                            void *new_ctx = serv->alloc(serv, client_sock);
                            if(new_ctx == NULL)
                            {
                                server_close(serv, idx);
                            }
                            else
                            {
                                serv->ctxs[idx].type = SOCK_CLIENT;
                                serv->ctxs[idx].data = new_ctx;
                            }
                        }
                    }
                }
                else
                {
                    if(serv->data_cb != NULL)
                    {
                        if(serv->data_cb(fds[i].fd, serv->ctxs[i].data) == -1)
                        {
                            server_close(serv, i);
                        }
                    }
                }
            }
            else if(fds[i].revents & POLLHUP || fds[i].revents & POLLERR) // For some reason, this never gets hit?
            {
                server_close(serv, i);
            }
        }

        if(serv->compact_needed) compact(serv);
    }

    // Clean up.
    for(unsigned int i = 0; i < serv->nfds; i++)
    {
        if(fds[i].fd != -1) socClose(fds[i].fd);
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