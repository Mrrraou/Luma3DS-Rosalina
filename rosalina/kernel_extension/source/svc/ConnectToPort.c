#include "svc/ConnectToPort.h"
#include "memory.h"
#include "ipc.h"

Result ConnectToPortHook(u32 dummy, const char *name)
{
    u32 regs[3];
    char portName[9] = {0};
    Result res = 0;

    usrToKernelStrncpy(portName, name, 8);

    res = ((Result (*)(u32, const char *))officialSVCs[0x2D])(dummy, name);
    getR1toR3(regs);

    if(strncmp(portName, "srv:", 4) == 0)
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, (Handle) regs[0]);
        if(session != NULL)
        {
            TracedService_Add(&srvPort, session);
            session->vtable->DecrementReferenceCount(session);
        }
    }

    return setR0toR3(res, regs[0], regs[1], regs[2]);
}
