#include <3ds.h>
#include <string.h>
#include "fsreg.h"
#include "services.h"

static Handle fsregHandle;
static int fsregRefCount;

Result fsregInit(void)
{
    Result ret = 0;

    if(AtomicPostIncrement(&fsregRefCount))
        return 0;

    ret = svcGetSystemInfo((s64 *)&fsregHandle, 0x20000, 0);

    if(R_FAILED(ret))
        AtomicDecrement(&fsregRefCount);
    return ret;
}

void fsregExit(void)
{
    if(AtomicDecrement(&fsregRefCount) || !fsregHandle)
        return;
    svcCloseHandle(fsregHandle);
}

Handle fsregGetHandle(void)
{
    return fsregHandle;
}

Result fsregSetupPermissions(void)
{
    u32 pid;
    Result res;
    FS_ProgramInfo info;
    u32 storage[8] = {0};

    storage[6] = 0x800 | 0x400 | 0x80 | 0x1; // SDMC access and NAND access flag
    info.programId = 0x0004013000006902LL; // Rosalina TID
    info.mediaType = MEDIATYPE_NAND;

    if(R_SUCCEEDED(res = svcGetProcessId(&pid, 0xFFFF8001))) // 0xFFFF8001 is an handle to the active process
        res = FSREG_Register(pid, 0xFFFF000000000000LL, &info, (u8*)storage);

    return res;
}

Result FSREG_CheckHostLoadId(u64 prog_handle)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x406,2,0); // 0x4060080
    cmdbuf[1] = (u32) (prog_handle);
    cmdbuf[2] = (u32) (prog_handle >> 32);

    Result ret = 0;
    if(R_FAILED(ret = svcSendSyncRequest(fsregHandle)))
        return ret;

    return cmdbuf[1];
}

Result FSREG_LoadProgram(u64 *prog_handle, FS_ProgramInfo *title)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x404,4,0); // 0x4040100
    memcpy(&cmdbuf[1], &title->programId, sizeof(u64));
    *(u8 *)&cmdbuf[3] = title->mediaType;
    memcpy(((u8 *)&cmdbuf[3])+1, &title->padding, 7);

    Result ret = 0;
    if(R_FAILED(ret = svcSendSyncRequest(fsregHandle)))
        return ret;
    *prog_handle = *(u64 *)&cmdbuf[2];

    return cmdbuf[1];
}

Result FSREG_GetProgramInfo(exheader_header *exheader, u32 entry_count, u64 prog_handle)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x403,3,0); // 0x40300C0
    cmdbuf[1] = entry_count;
    *(u64 *)&cmdbuf[2] = prog_handle;
    cmdbuf[64] = ((entry_count << 10) << 14) | 2;
    cmdbuf[65] = (u32) exheader;

    Result ret = 0;
    if(R_FAILED(ret = svcSendSyncRequest(fsregHandle)))
    return ret;

    return cmdbuf[1];
}

Result FSREG_UnloadProgram(u64 prog_handle)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x405,2,0); // 0x4050080
    cmdbuf[1] = (u32) (prog_handle);
    cmdbuf[2] = (u32) (prog_handle >> 32);

    Result ret = 0;
    if(R_FAILED(ret = svcSendSyncRequest(fsregHandle)))
        return ret;

    return cmdbuf[1];
}

Result FSREG_Unregister(u32 pid)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x402,1,0); // 0x4020040
    cmdbuf[1] = pid;

    Result ret = 0;
    if(R_FAILED(ret = svcSendSyncRequest(fsregHandle)))
        return ret;

    return cmdbuf[1];
}

Result FSREG_Register(u32 pid, u64 prog_handle, FS_ProgramInfo *info, void *storageinfo)
{
    u32 *cmdbuf = getThreadCommandBuffer();

    cmdbuf[0] = IPC_MakeHeader(0x401,0xf,0); // 0x40103C0
    cmdbuf[1] = pid;
    *(u64 *)&cmdbuf[2] = prog_handle;
    memcpy(&cmdbuf[4], &info->programId, sizeof(u64));
    *(u8 *)&cmdbuf[6] = info->mediaType;
    memcpy(((u8 *)&cmdbuf[6])+1, &info->padding, 7);
    memcpy((u8 *)&cmdbuf[8], storageinfo, 32);

    Result ret = 0;
    if(R_FAILED(ret = svcSendSyncRequest(fsregHandle)))
        return ret;

    return cmdbuf[1];
}
