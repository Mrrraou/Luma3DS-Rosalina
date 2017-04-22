#include "svc/CopyHandle.h"
#include "memory.h"

Result CopyHandle(Handle *outHandle, Handle outProcessHandle, Handle inHandle, Handle inProcessHandle)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KProcess *inProcess, *outProcess;
    Result res;

    if(inProcessHandle == CUR_PROCESS_HANDLE)
    {
        inProcess = currentCoreContext->objectContext.currentProcess;
        KAutoObject__AddReference((KAutoObject *)inProcess);
    }
    else
        inProcess = KProcessHandleTable__ToKProcess(handleTable, inProcessHandle);

    if(inProcess == NULL)
        return 0xD8E007F7; // invalid handle

    if(outProcessHandle == CUR_PROCESS_HANDLE)
    {
        outProcess = currentCoreContext->objectContext.currentProcess;
        KAutoObject__AddReference((KAutoObject *)outProcess);
    }
    else
        outProcess = KProcessHandleTable__ToKProcess(handleTable, outProcessHandle);

    if(outProcess == NULL)
    {
        ((KAutoObject *)inProcess)->vtable->DecrementReferenceCount((KAutoObject *)inProcess);
        return 0xD8E007F7; // invalid handle
    }

    KAutoObject *obj = KProcessHandleTable__ToKAutoObject(handleTableOfProcess(inProcess), inHandle);
    if(obj == NULL)
    {
        ((KAutoObject *)inProcess)->vtable->DecrementReferenceCount((KAutoObject *)inProcess);
        ((KAutoObject *)outProcess)->vtable->DecrementReferenceCount((KAutoObject *)outProcess);
        return 0xD8E007F7; // invalid handle
    }

    res = createHandleForProcess(outHandle, outProcess, obj);

    obj->vtable->DecrementReferenceCount(obj);
    ((KAutoObject *)inProcess)->vtable->DecrementReferenceCount((KAutoObject *)inProcess);
    ((KAutoObject *)outProcess)->vtable->DecrementReferenceCount((KAutoObject *)outProcess);

    return res;
}
