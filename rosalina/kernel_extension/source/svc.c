#include "svc.h"
#include "svc/CloseHandle.h"
#include "svc/GetProcessInfo.h"
#include "svc/GetSystemInfo.h"
#include "svc/ConnectToPort.h"
#include "svc/SendSyncRequest.h"
#include "svc/Break.h"
#include "svc/KernelSetState.h"

void *officialSVCs[0x7E] = {NULL};
KAutoObject *srvSessions[0x40] = {NULL};
KAutoObject *fsREGSessions[2] = {NULL};

TracedService tracedServices[1] =
{
    {"fs:REG", fsREGSessions, 2}
};

void *svcHook(u8 *pageEnd)
{
    KProcess *currentProcess = currentCoreContext->objectContext.currentProcess;
    u64 titleId = codeSetOfProcess(currentProcess)->titleId;
    u32 highTitleId = (u32)(titleId >> 32), lowTitleId = (u32)titleId;
    while(rosalinaState != 0 && *KPROCESS_GET_PTR(currentProcess, processId) >= 6 &&
      (highTitleId != 0x00040130 || (highTitleId == 0x00040130 && (lowTitleId == 0x1A02 || lowTitleId == 0x1C02))))
        yield();//WaitSynchronization1(rosalinaSyncObj, currentCoreContext->objectContext.currentThread, rosalinaSyncObj, -1LL);

    u32 svcId = *(u8 *)(pageEnd - 0xB5);
    switch(svcId)
    {
        case 0x23:
            return CloseHandleHook;
        case 0x2A:
            return GetSystemInfoHook;
        case 0x2B:
            return GetProcessInfoHook;
        case 0x2D:
            return ConnectToPortHook;
        case 0x32:
            return SendSyncRequestHook;
        case 0x3C:
            return BreakHook;
        case 0x7C:
            return KernelSetStateHook;
        default:
            return (svcId <= 0x7D) ? officialSVCs[svcId] : NULL;
    }
}
