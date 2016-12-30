#include "svc.h"
#include "svc/Break.h"
#include "svc/GetProcessInfo.h"
#include "svc/GetSystemInfo.h"
#include "svc/KernelSetState.h"

void *officialSVCs[0x7E] = {NULL};

// regs: r0-r7, r12
// user regs: r8-r11, sp, lr, pc+2/4, cspr

void *svcHook(u8 *pageEnd)
{
    u64 titleId = codeSetOfProcess(currentProcess)->titleId;
    while(rosalinaState != 0 && currentProcess->processId >= 6 && (u32)(titleId >> 32) != 0x00040130) yield();

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
            return (svcId <= 0x7D) ? officialSVCs[svcID] : NULL;
    }
}
