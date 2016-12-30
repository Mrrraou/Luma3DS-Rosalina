#include "GetProcessInfo.h"

Result GetProcessInfoHook(u32 dummy, Handle processHandle, u32 type)
{
    u64 out = 0;
    Result res = 0;

    if(type == 0x10000 || type == 0x10001)
    {
        KProcess *process = KProcessHandleTable__ToKProcess(currentKProcess->handleTable, processHandle);

        if(process == NULL)
            res = 0xD8E007F7; // invalid handle

        else if(type == 0x10000)
            memcpy(&out, codeSetOfProcess(process)->processName, 8);

        else
            out = codeSetOfProcess(process)->titleId;

        return setR0toR3(res, (u32)out >> 32, (u32)out);
    }

    else
        return ((Result (*) (u32, Handle, u32))officialSVCs[0x2B])(dummy, processHandle, type);
}
