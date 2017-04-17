#pragma once

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "utils.h"

#define MAX_SESSION     345

// the structure of sessions is apparently not the same on older versions...

typedef struct ClientSessionInfo
{
    KAutoObject *session;
    char name[12];
} ClientSessionInfo;

typedef struct LangemuAttributes
{
    u64 titleId;
    u8 region, language;
} LangemuAttributes;


extern KObjectMutex processLangemuObjectMutex;
extern LangemuAttributes processLangemuAttributes[0x40];

ClientSessionInfo *ClientSessionInfo_Lookup(KAutoObject *session);
ClientSessionInfo *ClientSessionInfo_FindFirst(const char *name);
void ClientSessionInfo_ChangeVtable(KAutoObject *session);
void ClientSessionInfo_Add(KAutoObject *session, const char *name);
void ClientSessionInfo_Remove(KAutoObject *session);

bool doLangEmu(bool region, u32 *cmdbuf);
Result doPublishToProcessHook(Handle handle, u32 *cmdbuf);
