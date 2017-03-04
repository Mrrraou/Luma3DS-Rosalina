#include "svc.h"
#include "svc/GetProcessInfo.h"
#include "svc/GetThreadInfo.h"
#include "svc/GetSystemInfo.h"
#include "svc/GetCFWInfo.h"
#include "svc/ConnectToPort.h"
#include "svc/SendSyncRequest.h"
#include "svc/Break.h"
#include "svc/RestrictGpuDma.h"
#include "svc/Backdoor.h"
#include "svc/KernelSetState.h"
#include "svc/MapProcessMemoryWithSource.h"

void *officialSVCs[0x7E] = {NULL};

void *svcHook(u8 *pageEnd)
{
    KProcess *currentProcess = currentCoreContext->objectContext.currentProcess;
    u64 titleId = codeSetOfProcess(currentProcess)->titleId;
    u32 highTitleId = (u32)(titleId >> 32), lowTitleId = (u32)titleId;
    while(rosalinaState != 0 && *KPROCESS_GET_PTR(currentProcess, processId) >= 6 &&
      (highTitleId != 0x00040130 || (highTitleId == 0x00040130 && (lowTitleId == 0x1A02 || lowTitleId == 0x1C02))))
        yield();

    u32 svcId = *(u8 *)(pageEnd - 0xB5);
    switch(svcId)
    {
        case 0x2A:
            return GetSystemInfoHookWrapper;
        case 0x2B:
            return GetProcessInfoHookWrapper;
        case 0x2C:
            return GetThreadInfoHookWrapper;
        case 0x2D:
            return ConnectToPortHookWrapper;
        case 0x2E:
            return GetCFWInfo; // DEPRECATED
        case 0x32:
            return SendSyncRequestHook;
        case 0x3C:
            return (debugOfProcess(currentProcess) != NULL) ? officialSVCs[0x3C] : (void *)Break;
        case 0x59:
            return RestrictGpuDma;
        case 0x7B:
            return Backdoor;
        case 0x7C:
            return KernelSetStateHook;

        case 0x80:
            return CustomBackdoor;
        case 0x81:
            return convertVAToPA;
        case 0x82:
            return flushDataCacheRange;
        case 0x83:
            return flushEntireDataCache;
        case 0x84:
            return invalidateInstructionCacheRange;
        case 0x85:
            return invalidateEntireInstructionCache;
        case 0x86:
            return MapProcessMemoryWithSource;

        default:
            return (svcId <= 0x7D) ? officialSVCs[svcId] : NULL;
    }
}
