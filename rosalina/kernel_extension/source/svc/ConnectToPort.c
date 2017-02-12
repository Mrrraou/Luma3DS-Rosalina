#include "svc/ConnectToPort.h"
#include "memory.h"

Result ConnectToPortHook(const char *name)
{
    u32 regs[3];
    char portName[9] = {0};

    usrStrncpy(portName, name, 8);

    if(strncmp(portName, "srvR", 4) == 0)
    {
        u32 *sp = (u32 *)((u8 *)currentCoreContext->objectContext.currenThread->endOfThreadContext - 0xD8); // user sp
        strncpy(portName, "srv:", 4);
        strncpy(sp - 8, "srv:", 4);
        Result res = ((Result (*)(const char *))officialSVCs[0x2D])(sp - 8); // "should be fine" (tm) at least for kernel-launched modules
    }

    getR1toR3(regs);

    if(strncmp(portName, "srv:", 4) == 0)
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, (Handle) regs[1]);
        if(session != NULL)
        {
            session->vtable->DecrementReferenceCount(session);

            addToSessionArray(session, srvSessions, 0x40);
        }
    }

    return setR0toR3(res, regs[0], regs[1], regs[2]);
}
