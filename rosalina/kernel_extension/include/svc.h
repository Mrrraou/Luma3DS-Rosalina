#pragma once

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "utils.h"

extern void *officialSVCs[0x7E];

static inline Result createHandleForThisProcess(Handle *out, KAutoObject *obj)
{
    u8 token;
    if(kernelVersion >= SYSTEM_VERSION(2, 46, 0))
    {
        KClassToken tok;
        obj->vtable->GetClassToken(&tok, obj);
        token = tok.flags;
    }
    else
        token = (u8)(u32)(obj->vtable->GetClassName(obj));

    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    return KProcessHandleTable__CreateHandle(handleTable, out, obj, token);
}

static inline void yield(void)
{
    ((void (*)(s64))officialSVCs[0x0A])(25*1000*1000);
}

void svcDefaultHandler(u8 svcId);
void *svcHook(u8 *pageEnd);
