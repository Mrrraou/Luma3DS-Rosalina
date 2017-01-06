#include "svc.h"
#include "svc/Break.h"
#include "svc/GetProcessInfo.h"
#include "svc/GetSystemInfo.h"
#include "svc/KernelSetState.h"

void *officialSVCs[0x7E] = {NULL};

void *svcHook(u8 *pageEnd)
{
    KProcess *currentProcess = currentCoreContext->objectContext.currentProcess;
    u64 titleId = codeSetOfProcess(currentProcess)->titleId;
    //while(rosalinaState != 0 && *KPROCESS_GET_PTR(currentProcess, processId) >= 6 && (u32)(titleId >> 32) != 0x00040130) yield();

    u32 svcId = *(u8 *)(pageEnd - 0xB5);
    switch(svcId)
    {
        case 0x2A:
            return GetSystemInfoHook;
        case 0x2B:
            return GetProcessInfoHook;
        case 0x3C:
            return BreakHook;
        case 0x7C:
            return KernelSetStateHook;
        default:
            return (svcId <= 0x7D) ? officialSVCs[svcId] : NULL;
    }
}
