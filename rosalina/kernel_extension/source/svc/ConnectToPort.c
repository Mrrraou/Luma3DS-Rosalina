#include "svc/ConnectToPort.h"
#include "memory.h"

Result ConnectToPortHook(u32 dummy, const char *name)
{
    u32 regs[3];
    char portName[9] = {0};
    Result res = 0;

    usrStrncpy(portName, name, 8);

    if(strncmp(portName, "srvR", 4) == 0)
    {
        char *sp = *(char **)((u8 *)currentCoreContext->objectContext.currentThread->endOfThreadContext - 0xD8); // user sp
        strncpy(portName, "srv:", 4);
        strncpy(sp - 8, "srv:", 4);
        name = sp - 8; // "should be fine" (tm) at least for kernel-launched modules
    }

    res = ((Result (*)(u32, const char *))officialSVCs[0x2D])(dummy, name);
    getR1toR3(regs);

    if(strncmp(portName, "srv:", 4) == 0)
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, (Handle) regs[0]);
        if(session != NULL)
        {
            session->vtable->DecrementReferenceCount(session);

            addToSessionArray(session, srvSessions, 0x40);
        }
    }

    return setR0toR3(res, regs[0], regs[1], regs[2]);
}
