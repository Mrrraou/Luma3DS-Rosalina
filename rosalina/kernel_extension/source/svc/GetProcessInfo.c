#include "svc/GetProcessInfo.h"
#include "memory.h"

Result GetProcessInfoHook(s64 *out, Handle processHandle, u32 type)
{
    Result res = 0;

    if(type >= 0x10000)
    {
        KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
        KProcess *process = KProcessHandleTable__ToKProcess(handleTable, processHandle);

        if(process == NULL)
            return 0xD8E007F7; // invalid handle

        switch(type)
        {
            case 0x10000:
                memcpy(out, codeSetOfProcess(process)->processName, 8);
                break;
            case 0x10001:
                *(u64 *)out = codeSetOfProcess(process)->titleId;
                break;
            case 0x10002:
                *out = codeSetOfProcess(process)->nbTextPages << 12;
                break;
            case 0x10003:
                *out = codeSetOfProcess(process)->nbRodataPages << 12;
                break;
            case 0x10004:
                *out = codeSetOfProcess(process)->nbRwPages << 12;
                break;
            default:
                res = 0xD8E007ED; // invalid enum value
                break;
        }

        KAutoObject *obj = (KAutoObject *)process;
        obj->vtable->DecrementReferenceCount(obj);
    }

    else
        res = GetProcessInfo(out, processHandle, type);

    return res;
}
