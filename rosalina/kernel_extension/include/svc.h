#pragma once

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "utils.h"

typedef struct TracedService
{
    const char *name;
    KAutoObject **sessions;
    u32 maxSessions;
    KObjectMutex mutex;
} TracedService;

typedef struct LangemuAttributes
{
    u64 titleId;
    u8 region, language;
} LangemuAttributes;

extern void *officialSVCs[0x7E];

extern KAutoObject *srvSessions[0x40];
extern TracedService srvPort;

extern KAutoObject *fsREGSessions[2];
extern KAutoObject *cfgUSessions[25], *cfgSSessions[25], *cfgISessions[25];
extern TracedService fsREGService;
extern TracedService cfgUService, cfgSService, cfgIService;

extern TracedService *tracedServices[4];

extern KObjectMutex processLangemuObjectMutex;
extern LangemuAttributes processLangemuAttributes[0x40];

static inline u32 TracedService_Lookup(const TracedService *this, const KAutoObject *session)
{
    u32 i;
    for(i = 0; i < this->maxSessions && this->sessions[i] != session; i++);
    return i;
}

static inline void TracedService_Add(TracedService *this, KAutoObject *session)
{
    KObjectMutex__Acquire(&this->mutex);

    u32 i;
    for(i = 0; i < this->maxSessions && this->sessions[i] != NULL; i++);
    if(i < this->maxSessions) this->sessions[i] = session;

    KObjectMutex__Release(&this->mutex);
}

static inline void TracedService_Remove(TracedService *this, const KAutoObject *session)
{
    KObjectMutex__Acquire(&this->mutex);

    u32 i = TracedService_Lookup(this, session);
    if(i < this->maxSessions) this->sessions[i] = NULL;

    KObjectMutex__Release(&this->mutex);
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
