#include "svc/SendSyncRequest.h"

Result SendSyncRequestHook(Handle handle)
{

    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, handle);
    if(session != NULL)
    {
        session->vtable->DecrementReferenceCount(session);
        u32 *cmdbuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x80);

        switch (cmdbuf[0])
        {
            case 0x50100:
                u32 i;
                for(i = 0; i < 0x40 && srvSessions[i] != session; i++);
                if(i < 0x40)
                {

                }
        }

    }

    return ((Result (*)(Handle))officialSVCs[0x32])(handle);
}
