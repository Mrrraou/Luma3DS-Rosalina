#pragma once

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "utils.h"

#define MAX_SESSION     345

// the structure of sessions is apparently not the same on older versions...

typedef struct SessionInfo
{
    KSession *session;
    char name[12];
} SessionInfo;

typedef struct LangemuAttributes
{
    u64 titleId;
    u8 region, language;
} LangemuAttributes;


extern KRecursiveLock processLangemuLock;
extern LangemuAttributes processLangemuAttributes[0x40];

SessionInfo *SessionInfo_Lookup(KSession *session);
SessionInfo *SessionInfo_FindFirst(const char *name);
void SessionInfo_ChangeVtable(KSession *session);
void SessionInfo_Add(KSession *session, const char *name);
void SessionInfo_Remove(KSession *session);

bool doLangEmu(bool region, u32 *cmdbuf);
Result doPublishToProcessHook(Handle handle, u32 *cmdbuf);
