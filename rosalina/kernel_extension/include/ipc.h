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

// the structure of sessions is apparently not the same on older versions...

extern KAutoObject *srvSessions[0x40];
extern TracedService srvPort;

extern KAutoObject *srvPmSessions[2];
extern TracedService srvPmService;

extern KAutoObject *fsREGSessions[2];
extern KAutoObject *cfgUSessions[25], *cfgSSessions[25], *cfgISessions[25];
extern TracedService fsREGService;
extern TracedService cfgUService, cfgSService, cfgIService;

extern TracedService *tracedServices[5];

extern KObjectMutex processLangemuObjectMutex;
extern LangemuAttributes processLangemuAttributes[0x40];

static inline u32 TracedService_Lookup(const TracedService *this, const KAutoObject *session)
{
    u32 i;
    for(i = 0; i < this->maxSessions && this->sessions[i] != session; i++);
    return i;
}

void TracedService_ChangeClientSessionVtable(KAutoObject *obj);

static inline void TracedService_Add(TracedService *this, KAutoObject *session)
{
    KObjectMutex__Acquire(&this->mutex);

    u32 i;
    for(i = 0; i < this->maxSessions && this->sessions[i] != NULL; i++);
    if(i < this->maxSessions)
    {
        this->sessions[i] = session;
        TracedService_ChangeClientSessionVtable(session);
    }

    KObjectMutex__Release(&this->mutex);
}

static inline bool TracedService_Remove(TracedService *this, const KAutoObject *session)
{
    KObjectMutex__Acquire(&this->mutex);

    u32 i = TracedService_Lookup(this, session);
    if(i < this->maxSessions) this->sessions[i] = NULL;

    KObjectMutex__Release(&this->mutex);

    return i < this->maxSessions;
}

bool doLangEmu(bool region, u32 *cmdbuf);
Result doPublishToProcessHook(Handle handle, u32 *cmdbuf);
