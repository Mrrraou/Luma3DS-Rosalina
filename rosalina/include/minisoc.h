#pragma once

#include <errno.h>
#include <sys/socket.h>
#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/srv.h>
#include <3ds/services/soc.h>
#include <poll.h>

#define SYNC_ERROR ENODEV

extern Handle	SOCU_handle;
extern Handle	socMemhandle;

Result miniSocInit(u32 context_size);
Result miniSocExit(void);

s32 _net_convert_error(s32 sock_retval);

Result socSocket(Handle *out, int domain, int type, int protocol);
Result socBind(Handle sockfd, const struct sockaddr *addr, socklen_t addrlen);
Result socListen(Handle sockfd, int max_connections);
Result socAccept(Result sockfd, Handle *out, struct sockaddr *addr, socklen_t *addrlen);
Result socPoll(struct pollfd *fds, nfds_t nfds, int timeout);
int socClose(Handle sockfd);

ssize_t soc_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);