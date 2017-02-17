#include "svc/CloseHandle.h"
#include "memory.h"

Result CloseHandleHook(Handle handle)
{

    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, handle);
    if(session != NULL)
    {
        session->vtable->DecrementReferenceCount(session);

        if(session->refCount == 1)
            TracedService_Remove(&srvPort, session);
    }

    return ((Result (*)(Handle))officialSVCs[0x23])(handle);
}
