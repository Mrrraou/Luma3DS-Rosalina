#include "minisoc.h"
#include <sys/socket.h>
#include <3ds/ipc.h>
#include <3ds/os.h>
#include "memory.h"

static u32 soc_block_addr;
static size_t soc_block_size;

static Result SOCU_Initialize(Handle memhandle, u32 memsize)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x1,1,4); // 0x10044
	cmdbuf[1] = memsize;
	cmdbuf[2] = IPC_Desc_CurProcessHandle();
	cmdbuf[4] = IPC_Desc_SharedHandles(1);
	cmdbuf[5] = memhandle;

	ret = svcSendSyncRequest(SOCU_handle);
	if(ret != 0)
	{
		return ret;
	}

	return cmdbuf[1];
}

static Result SOCU_Shutdown(void)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x19,0,0); // 0x190000

	ret = svcSendSyncRequest(SOCU_handle);
	if(ret != 0)
	{
		return ret;
	}

	return cmdbuf[1];
}

Result miniSocInit(u32 context_size)
{
	Result ret = 0;

	soc_block_addr = 0x08000000;
	soc_block_size = context_size;

	u32 tmp = 0;
	ret = svcControlMemory(&tmp, soc_block_addr, 0, context_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);
	if(ret != 0) return ret;
	soc_block_addr = tmp;

	ret = svcCreateMemoryBlock(&socMemhandle, soc_block_addr, context_size, 0, 3);
	if(ret != 0) return ret;

	ret = srvGetServiceHandle(&SOCU_handle, "soc:U");
	if(ret != 0)
	{
		svcCloseHandle(socMemhandle);
		socMemhandle = 0;
		return ret;
	}

	ret = SOCU_Initialize(socMemhandle, context_size);
	if(ret != 0)
	{
		svcCloseHandle(socMemhandle);
		svcCloseHandle(SOCU_handle);
		socMemhandle = 0;
		SOCU_handle = 0;
		return ret;
	}

	return 0;
}

Result miniSocExit(void)
{
	Result ret = 0;

	svcCloseHandle(socMemhandle);
	socMemhandle = 0;

	u32 tmp;
	svcControlMemory(&tmp, soc_block_addr, soc_block_addr, soc_block_size, MEMOP_FREE, 0);

	ret = SOCU_Shutdown();

	svcCloseHandle(SOCU_handle);
	SOCU_handle = 0;

	return ret;
}

Result socSocket(Handle *out, int domain, int type, int protocol)
{
	int ret = 0;

	u32 *cmdbuf = getThreadCommandBuffer();

	// The protocol on the 3DS *must* be 0 to work
	// To that end, when appropriate, we will make the change for the user
	if (domain == AF_INET
	&& type == SOCK_STREAM
	&& protocol == IPPROTO_TCP) {
		protocol = 0; // TCP is the only option, so 0 will work as expected
	}
	if (domain == AF_INET
	&& type == SOCK_DGRAM
	&& protocol == IPPROTO_UDP) {
		protocol = 0; // UDP is the only option, so 0 will work as expected
	}

	cmdbuf[0] = IPC_MakeHeader(0x2,3,2); // 0x200C2
	cmdbuf[1] = domain;
	cmdbuf[2] = type;
	cmdbuf[3] = protocol;
	cmdbuf[4] = IPC_Desc_CurProcessHandle();

	ret = svcSendSyncRequest(SOCU_handle);
	if(ret != 0)
	{
		return ret;
	}

	ret = (int)cmdbuf[1];
	if(ret == 0) ret = cmdbuf[2];
	if(ret < 0)
	{
		return ret;
	}
	else
	{
		*out = cmdbuf[2];
		return 0;
	}
}

Result socBind(Handle sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	Result ret = 0;
	socklen_t tmp_addrlen = 0;
	u32 *cmdbuf = getThreadCommandBuffer();
	u8 tmpaddr[0x1c];

	memset_(tmpaddr, 0, 0x1c);

	if(addr->sa_family == AF_INET)
		tmp_addrlen = 8;
	else
		tmp_addrlen = 0x1c;

	if(addrlen < tmp_addrlen)
	{
		return -1;
	}

	tmpaddr[0] = tmp_addrlen;
	tmpaddr[1] = addr->sa_family;
	memcpy(&tmpaddr[2], &addr->sa_data, tmp_addrlen-2);

	cmdbuf[0] = IPC_MakeHeader(0x5,2,4); // 0x50084
	cmdbuf[1] = (u32)sockfd;
	cmdbuf[2] = (u32)tmp_addrlen;
	cmdbuf[3] = IPC_Desc_CurProcessHandle();
	cmdbuf[5] = IPC_Desc_StaticBuffer(tmp_addrlen,0);
	cmdbuf[6] = (u32)tmpaddr;

	ret = svcSendSyncRequest(SOCU_handle);
	if(ret != 0)
	{
		return ret;
	}

	ret = (int)cmdbuf[1];

	return ret;
}

Result socListen(Handle sockfd, int max_connections)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0x3,2,2); // 0x30082
	cmdbuf[1] = (u32)sockfd;
	cmdbuf[2] = (u32)max_connections;
	cmdbuf[3] = IPC_Desc_CurProcessHandle();

	ret = svcSendSyncRequest(SOCU_handle);
	if(ret != 0)
	{
		return ret;
	}

	ret = cmdbuf[1];

	return ret;
}

Result socAccept(Handle sockfd, Handle *out, struct sockaddr *addr, socklen_t *addrlen)
{
	Result ret = 0;
	int tmp_addrlen = 0x1c;

	u32 *cmdbuf = getThreadCommandBuffer();
	u8 tmpaddr[0x1c];
	u32 saved_threadstorage[2];

	memset_(tmpaddr, 0, 0x1c);

	cmdbuf[0] = IPC_MakeHeader(0x4,2,2); // 0x40082
	cmdbuf[1] = (u32)sockfd;
	cmdbuf[2] = (u32)tmp_addrlen;
	cmdbuf[3] = IPC_Desc_CurProcessHandle();

	u32 *staticbufs = getThreadStaticBuffers();
	saved_threadstorage[0] = staticbufs[0];
	saved_threadstorage[1] = staticbufs[1];

	staticbufs[0] = IPC_Desc_StaticBuffer(tmp_addrlen,0);
	staticbufs[1] = (u32)tmpaddr;

	ret = svcSendSyncRequest(SOCU_handle);

	staticbufs[0] = saved_threadstorage[0];
	staticbufs[1] = saved_threadstorage[1];

	if(ret != 0)
		return ret;

	ret = (int)cmdbuf[1];
	if(ret == 0)
		ret = _net_convert_error(cmdbuf[2]);

	if(ret >= 0 && addr != NULL)
	{
		addr->sa_family = tmpaddr[1];
		if(*addrlen > tmpaddr[0])
			*addrlen = tmpaddr[0];
		memcpy(addr->sa_data, &tmpaddr[2], *addrlen - 2);
	}

	if(ret >= 0)
		*out = (Handle)ret;

	return ret;
}

Result socPoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int ret = 0;
	nfds_t i;
	u32 size = sizeof(struct pollfd)*nfds;
	u32 *cmdbuf = getThreadCommandBuffer();
	u32 saved_threadstorage[2];

	if(nfds == 0) {
		return -1;
	}

	for(i = 0; i < nfds; ++i) {
		fds[i].revents = 0;
	}

	cmdbuf[0] = IPC_MakeHeader(0x14,2,4); // 0x140084
	cmdbuf[1] = (u32)nfds;
	cmdbuf[2] = (u32)timeout;
	cmdbuf[3] = IPC_Desc_CurProcessHandle();
	cmdbuf[5] = IPC_Desc_StaticBuffer(size,10);
	cmdbuf[6] = (u32)fds;

	u32 * staticbufs = getThreadStaticBuffers();
	saved_threadstorage[0] = staticbufs[0];
	saved_threadstorage[1] = staticbufs[1];

	staticbufs[0] = IPC_Desc_StaticBuffer(size,0);
	staticbufs[1] = (u32)fds;

	ret = svcSendSyncRequest(SOCU_handle);

	staticbufs[0] = saved_threadstorage[0];
	staticbufs[1] = saved_threadstorage[1];

	if(ret != 0) {
		return ret;
	}

	ret = (int)cmdbuf[1];
	if(ret == 0)
		ret = _net_convert_error(cmdbuf[2]);

	if(ret < 0) {
		return -1;
	}

	return ret;
}

int socClose(Handle sockfd)
{
	int ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(0xB,1,2); // 0xB0042
	cmdbuf[1] = (u32)sockfd;
	cmdbuf[2] = IPC_Desc_CurProcessHandle();

	ret = svcSendSyncRequest(SOCU_handle);
	if(ret != 0) {
		return ret;
	}

	ret = (int)cmdbuf[1];

	return ret;
}

ssize_t soc_recv(Handle sockfd, void *buf, size_t len, int flags)
{
	return soc_recvfrom(sockfd, buf, len, flags, NULL, 0);
}

ssize_t soc_send(Handle sockfd, const void *buf, size_t len, int flags)
{
	return soc_sendto(sockfd, buf, len, flags, NULL, 0);
}
