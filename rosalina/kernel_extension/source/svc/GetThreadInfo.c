#include "svc/GetThreadInfo.h"
#include "memory.h"

Result GetThreadInfoHook(s64 *out, Handle threadHandle, u32 type)
{
    Result res = 0;

    if(type == 0x10000) // Get TLS
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KThread *thread;

        if(threadHandle == CUR_THREAD_HANDLE)
        {
            thread = currentCoreContext->objectContext.currentThread;
            KAutoObject__AddReference(&thread->syncObject.autoObject);
        }
        else
            thread = KProcessHandleTable__ToKThread(handleTable, threadHandle);

        if(thread == NULL)
            return 0xD8E007F7; // invalid handle

        *out = (s64)(u64)(u32)thread->threadLocalStorage;

        KAutoObject *obj = (KAutoObject *)thread;
        obj->vtable->DecrementReferenceCount(obj);
    }

    else
        res = GetThreadInfo(out, threadHandle, type);

    return res;
}
