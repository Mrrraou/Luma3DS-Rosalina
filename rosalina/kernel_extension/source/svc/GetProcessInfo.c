#include "svc/GetProcessInfo.h"
#include "memory.h"

Result GetProcessInfoHook(u32 dummy UNUSED, Handle processHandle, u32 type)
{
    u64 out = 0; // should be s64 but who cares
    Result res = 0;

    if(type == 0x10000 || type == 0x10001)
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KProcess *process = KProcessHandleTable__ToKProcess(handleTable, processHandle);

        if(process == NULL)
            res = 0xD8E007F7; // invalid handle

        else if(type == 0x10000)
            memcpy(&out, codeSetOfProcess(process)->processName, 8);

        else
            out = codeSetOfProcess(process)->titleId;

        KAutoObject *obj = (KAutoObject *)process;
        if(process != NULL)
            obj->vtable->DecrementReferenceCount(obj);
    }

    else
        res = GetProcessInfo((s64 *)&out, processHandle, type);

    return setR0toR3(res, (u32)out, (u32)(out >> 32));
}
