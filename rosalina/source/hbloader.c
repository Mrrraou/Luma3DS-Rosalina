#include <3ds.h>
#include "hbloader.h"
#include "3dsx.h"
#include "menu.h"
#include "csvc.h"
#include "memory.h"

#define MAP_BASE 0x10000000

static inline void assertSuccess(Result res)
{
	if(R_FAILED(res))
		svcBreak(USERBREAK_PANIC);
}

static MyThread hbldrThread;
static u8 ALIGN(8) hbldrThreadStack[THREAD_STACK_SIZE];
static u16 hbldrTarget[PATH_MAX+1];

MyThread *hbldrCreateThread(void)
{
	MyThread_Create(&hbldrThread, hbldrThreadMain, hbldrThreadStack, THREAD_STACK_SIZE, 0x18, CORE_SYSTEM);
	return &hbldrThread;
}

static inline void error(u32* cmdbuf, Result rc)
{
	cmdbuf[0] = IPC_MakeHeader(0, 1, 0);
	cmdbuf[1] = rc;
}

static u16 *u16_strncpy(u16 *dest, const u16 *src, u32 size)
{
	u32 i;
	for (i = 0; i < size && src[i] != 0; i++)
		dest[i] = src[i];
	while (i < size)
		dest[i++] = 0;

    return dest;
}

static void hbldrHandleCommands(void)
{
	Result res;
	IFile file;
	u32 *cmdbuf = getThreadCommandBuffer();
	switch (cmdbuf[0] >> 16)
	{
		case 1:
		{
			if (cmdbuf[0] != IPC_MakeHeader(1, 6, 0))
			{
				error(cmdbuf, 0xD9001830);
				break;
			}

			u32 baseAddr = cmdbuf[1];
			u32 flags = cmdbuf[2] & 0xF00;
			u64 tid = (u64)cmdbuf[3] | ((u64)cmdbuf[4]<<32);
			char name[8];
			memcpy(name, &cmdbuf[5], sizeof(name));

			if (hbldrTarget[0] == 0)
			{
				u16_strncpy(hbldrTarget, u"/boot.3dsx", PATH_MAX);
				ldrArgvBuf[0] = 1;
				strncpy((char*)&ldrArgvBuf[1], "sdmc:/boot.3dsx", sizeof(ldrArgvBuf)-4);
			}

			res = IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_UTF16, hbldrTarget), FS_OPEN_READ);
			hbldrTarget[0] = 0;
			if (R_FAILED(res))
			{
				error(cmdbuf, res);
				break;
			}

			u32 totalSize = 0;
			res = Ldr_Get3dsxSize(&totalSize, &file);
			if (R_FAILED(res))
			{
				IFile_Close(&file);
				error(cmdbuf, res);
				break;
			}

			u32 tmp = 0;
			res = svcControlMemoryEx(&tmp, MAP_BASE, 0, totalSize, MEMOP_ALLOC | flags, MEMPERM_READ | MEMPERM_WRITE, true);
			if (R_FAILED(res))
			{
				IFile_Close(&file);
				error(cmdbuf, res);
				break;
			}

			Handle hCodeset = Ldr_CodesetFrom3dsx(name, (u32*)MAP_BASE, baseAddr, &file, tid);
			IFile_Close(&file);

			if (!hCodeset)
			{
				svcControlMemory(&tmp, MAP_BASE, 0, totalSize, MEMOP_FREE, 0);
				error(cmdbuf, MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_LDR, RD_NOT_FOUND));
				break;
			}

			cmdbuf[0] = IPC_MakeHeader(1, 1, 2);
			cmdbuf[1] = 0;
			cmdbuf[2] = IPC_Desc_MoveHandles(1);
			cmdbuf[3] = hCodeset;
			break;
		}
		case 2:
		{
			if (cmdbuf[0] != IPC_MakeHeader(2, 0, 2) || (cmdbuf[1] & 0x3FFF) != 0x0002)
			{
				error(cmdbuf, 0xD9001830);
				break;
			}
			ssize_t units = utf8_to_utf16(hbldrTarget, (const uint8_t*)cmdbuf[2], PATH_MAX);
			if (units < 0 || units > PATH_MAX)
			{
				hbldrTarget[0] = 0;
				error(cmdbuf, 0xD9001830);
				break;
			}

			hbldrTarget[units] = 0;
			cmdbuf[0] = IPC_MakeHeader(2, 1, 0);
			cmdbuf[1] = 0;
			break;
		}
		case 3:
		{
			if (cmdbuf[0] != IPC_MakeHeader(3, 0, 2) || (cmdbuf[1] & 0x3FFF) != (0x2 | (1<<10)))
			{
				error(cmdbuf, 0xD9001830);
				break;
			}
			// No need to do anything - the kernel already copied the data to our buffer
			cmdbuf[0] = IPC_MakeHeader(3, 1, 0);
			cmdbuf[1] = 0;
			break;
		}
		default:
		{
			error(cmdbuf, 0xD900182F);
			break;
		}
	}
}

void hbldrThreadMain(void)
{
	Handle handles[2];
	Handle serverHandle, clientHandle, sessionHandle = 0;

	u32 replyTarget = 0;
	s32 index;

	char ipcBuf[PATH_MAX+1];
	u32* bufPtrs = getThreadStaticBuffers();
	memset(bufPtrs, 0, 16*2*4);
	bufPtrs[0] = IPC_Desc_StaticBuffer(sizeof(ipcBuf), 0);
	bufPtrs[1] = (u32)ipcBuf;
	bufPtrs[2] = IPC_Desc_StaticBuffer(sizeof(ldrArgvBuf), 1);
	bufPtrs[3] = (u32)ldrArgvBuf;

	Result res;
	u32 *cmdbuf = getThreadCommandBuffer();

	assertSuccess(svcCreatePort(&serverHandle, &clientHandle, "hb:ldr", 1));

	do
	{
		handles[0] = serverHandle;
		handles[1] = sessionHandle;

		if(replyTarget == 0) // k11
			cmdbuf[0] = 0xFFFF0000;
		res = svcReplyAndReceive(&index, handles, sessionHandle == 0 ? 1 : 2, replyTarget);

		if(R_FAILED(res))
		{
			if((u32)res == 0xC920181A) // session closed by remote
			{
				svcCloseHandle(sessionHandle);
				sessionHandle = 0;
				replyTarget = 0;
			}

			else
				svcBreak(USERBREAK_PANIC);
		}

		else
		{
			if(index == 0)
			{
				Handle session;
				assertSuccess(svcAcceptSession(&session, serverHandle));

				if(sessionHandle == 0)
					sessionHandle = session;
				else
					svcCloseHandle(session);
			}
			else
			{
				hbldrHandleCommands();
				replyTarget = sessionHandle;
			}
		}
	}
	while(!terminationRequest);

	svcCloseHandle(sessionHandle);
	svcCloseHandle(clientHandle);
	svcCloseHandle(serverHandle);
}
