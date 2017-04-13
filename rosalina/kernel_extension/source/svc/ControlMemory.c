#include "svc/ControlMemory.h"

Result ControlMemoryHook(u32 *addrOut, u32 addr0, u32 addr1, u32 size, MemOp op, MemPerm perm)
{
    KProcess *currentProcess = currentCoreContext->objectContext.currentProcess;
    return ControlMemory(addrOut, addr0, addr1, size, op, perm, idOfProcess(currentProcess) == 1);
}

