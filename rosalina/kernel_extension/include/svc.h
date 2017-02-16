#pragma once

#include "types.h"
#include "globals.h"
#include "kernel.h"

typedef struct TracedService
{
    const char *name;
    KAutoObject **sessions;
    u32 maxSessions;
} TracedService;

typedef struct LangemuAttributes
{
    u64 titleId;
    u8 region, language;
} LangemuAttributes;

extern void *officialSVCs[0x7E];

extern KAutoObject *srvSessions[0x40];

extern TracedService tracedServices[4];
extern KAutoObject *fsREGSessions[2];
extern KAutoObject *cfgUSessions[25], *cfgSSessions[25], *cfgISessions[25];

extern LangemuAttributes processLangemuAttributes[0x40];

static inline u32 lookUpInSessionArray(KAutoObject *session, KAutoObject **sessions, u32 len)
{
    u32 i;
    for(i = 0; i < len && sessions[i] != session; i++);
    return i;
}

static inline void addToSessionArray(KAutoObject *session, KAutoObject **sessions, u32 len)
{
    u32 i;
    for(i = 0; i < len && sessions[i] != NULL; i++);
    if(i < len) sessions[i] = session;
}

static inline void removeFromSessionArray(KAutoObject *session, KAutoObject **sessions, u32 len)
{
    u32 i = lookUpInSessionArray(session, sessions, len);
    if(i < len) sessions[i] = NULL;
}

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
