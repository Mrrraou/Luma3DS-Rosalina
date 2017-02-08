#include "svc/ConnectToPort.h"
#include "memory.h"

Result ConnectToPortHook(const char *name)
{
    u32 regs[3];

    Result res = ((Result (*)(const char *))officialSVCs[0x2D])(name);
    getR1toR3(regs);

    if(strncmp(name, "srv:", 4) == 0)
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, (Handle) regs[1]);
        if(session != NULL)
        {
            session->vtable->DecrementReferenceCount(session);

            u32 i;
            for(i = 0; i < 0x40 && srvSessions[i] == NULL; i++);
            if(i != 0x40) srvSessions[i] = session;
        }
    }

    return setR0toR3(res, regs[0], regs[1], regs[2]);
}
