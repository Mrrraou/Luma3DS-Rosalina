#include "svc/GetThreadInfo.h"
#include "memory.h"

Result GetThreadInfoHook(u32 dummy UNUSED, Handle threadHandle, u32 type)
{
    u64 out = 0; // should be s64 but who cares
    Result res = 0;

    if(type == 0x10000) // Get TLS
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KThread *thread = KProcessHandleTable__ToKThread(handleTable, threadHandle);

        if(thread == NULL)
            res = 0xD8E007F7; // invalid handle

        out = (u64)(u32)thread->threadLocalStorage;


        KAutoObject *obj = (KAutoObject *)thread;
        if(thread != NULL)
            obj->vtable->DecrementReferenceCount(obj);
    }

    else
        res = GetThreadInfo((s64 *)&out, threadHandle, type);

    return setR0toR3(res, (u32)out, (u32)(out >> 32));
}
