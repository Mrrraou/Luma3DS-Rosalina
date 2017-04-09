#include "svc/KernelSetState.h"
#include "synchronization.h"
#include "ipc.h"
#include "memory.h"

#define MAX_DEBUG 3

static u32 nbEnabled = 0;
static u32 maskedPids[MAX_DEBUG];
static u32 masks[MAX_DEBUG][8] = {0};

s32 rosalinaState;

bool shouldSignalSyscallDebugEvent(const KProcess *process, u8 svcId)
{
    u32 pid = KPROCESS_GET_RVALUE(process, processId);
    u32 id;
    for(id = 0; id < nbEnabled && maskedPids[id] != pid; id++);
    if(id == MAX_DEBUG)
        return false;
    else
        return ((masks[id][svcId / 32] >> (31 - (svcId % 32))) & 1) != 0;
}

Result SetSyscallDebugEventMask(u32 pid, bool enable, const u32 *mask)
{
    static KObjectMutex syscallDebugEventMaskMutex = {NULL};

    u32 tmpMask[8];
    if(enable && nbEnabled == MAX_DEBUG)
        return 0xC86018FF; // Out of resource (255)

    if(enable && !usrToKernelMemcpy8(&tmpMask, mask, 32))
        return 0xE0E01BF5;

    KObjectMutex__Acquire(&syscallDebugEventMaskMutex);

    if(enable)
    {
        maskedPids[nbEnabled] = pid;
        memcpy(&masks[nbEnabled++], tmpMask, 32);
    }
    else
    {
        u32 id;
        for(id = 0; id < nbEnabled && maskedPids[id] != pid; id++);
        if(id == nbEnabled)
        {
            KObjectMutex__Release(&syscallDebugEventMaskMutex);
            return 0xE0E01BFD; // out of range (it's not fully technically correct but meh)
        }

        for(u32 i = id; i < nbEnabled - 1; i++)
        {
            maskedPids[i] = maskedPids[i + 1];
            memcpy(&masks[i], &masks[i + 1], 32);
        }
        maskedPids[--nbEnabled] = 0;
        memset(&masks[nbEnabled], 0, 32);
    }

    KObjectMutex__Release(&syscallDebugEventMaskMutex);
    return 0;
}

Result KernelSetStateHook(u32 type, u32 varg1, u32 varg2, u32 varg3)
{
    Result res = 0;

    switch(type)
    {
        case 0x10000:
        {
            atomicStore32(&rosalinaState, (s32) varg1);
            break;
        }
        case 0x10001:
        {
            KObjectMutex__Acquire(&processLangemuObjectMutex);

            u32 i;
            for(i = 0; i < 0x40 && processLangemuAttributes[i].titleId != 0ULL; i++);
            if(i < 0x40)
            {
                processLangemuAttributes[i].titleId = ((u64)varg3 << 32) | (u32)varg2;
                processLangemuAttributes[i].region = (u8)(varg1 >> 8);
                processLangemuAttributes[i].language = (u8)varg1;
            }
            else
                res = 0xD8609013;

            KObjectMutex__Release(&processLangemuObjectMutex);

            break;
        }
        case 0x10002:
        {
            res = SetSyscallDebugEventMask(varg1, (bool)varg2, (const u32 *)varg3);
            break;
        }

        default:
        {
            res = ((Result (*)(u32, u32, u32, u32))officialSVCs[0x7C])(type, varg1, varg2, varg3);
            break;
        }
    }

    return res;
}
