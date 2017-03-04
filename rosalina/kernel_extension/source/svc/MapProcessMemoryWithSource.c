#include "svc/MapProcessMemoryWithSource.h"

Result MapProcessMemoryWithSource(Handle processHandle, void *dst, void *src, u32 size)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KProcessHwInfo *currentHwInfo = hwInfoOfProcess(currentCoreContext->objectContext.currentProcess);
    KProcess *process = KProcessHandleTable__ToKProcess(handleTable, processHandle);

    if(process == NULL)
        return 0xD8E007F7;

    Result res = KProcessHwInfo__MapProcessMemory(currentHwInfo, hwInfoOfProcess(process), dst, src, size >> 12);

    KAutoObject *obj = (KAutoObject *)process;
    obj->vtable->DecrementReferenceCount(obj);

    return res;
}
