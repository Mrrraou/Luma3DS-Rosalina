#include "svc/ConnectToPort.h"
#include "memory.h"
#include "ipc.h"

Result ConnectToPortHook(u32 dummy UNUSED, const char *name)
{
    Handle out;
    char portName[9] = {0};
    Result res = 0;

    usrToKernelStrncpy(portName, name, 8);

    res = ConnectToPort(&out, name);

    if(strncmp(portName, "srv:", 4) == 0)
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, out);
        if(session != NULL)
        {
            TracedService_Add(&srvPort, session);
            session->vtable->DecrementReferenceCount(session);
        }
    }

    return setR0toR3(res, out);
}
