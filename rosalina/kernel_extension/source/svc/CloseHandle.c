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
        {
            u32 i = lookUpInSessionArray(session, srvSessions, 0x40);
            if(i != 0x40) srvSessions[i] = NULL;
        }
    }

    return ((Result (*)(Handle))officialSVCs[0x23])(handle);
}
