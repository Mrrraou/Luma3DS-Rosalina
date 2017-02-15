#include "svc/SendSyncRequest.h"
#include "memory.h"

Result SendSyncRequestHook(Handle handle)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, handle);

    TracedService *traced = NULL;
    u32 *cmdbuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x80);

    if(session != NULL)
    {
        session->vtable->DecrementReferenceCount(session);

        switch (cmdbuf[0])
        {
            case 0x50100:
            {
                u32 index = lookUpInSessionArray(session, srvSessions, 0x40);
                if(index < 0x40)
                {
                    const char *serviceName = (const char *)(cmdbuf + 1);
                    for(u32 i = 0; i < 1; i++)
                    {
                        if(strncmp(serviceName, tracedServices[i].name, 8) == 0)
                            traced = tracedServices + i;
                    }
                }
                break;
            }
        }

    }

    Result res = ((Result (*)(Handle))officialSVCs[0x32])(handle);
    if(traced != NULL)
    {
        session = KProcessHandleTable__ToKAutoObject(handleTable, (Handle)cmdbuf[3]);
        if(session != NULL)
        {
            session->vtable->DecrementReferenceCount(session);
            addToSessionArray(session, traced->sessions, traced->maxSessions);
        }
    }

    return res;
}
