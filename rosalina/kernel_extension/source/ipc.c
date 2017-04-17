#include "ipc.h"
#include "memory.h"

static ClientSessionInfo sessionInfos[MAX_SESSION] = { {NULL} };
static u32 nbActiveSessions = 0;
static KObjectMutex sessionInfosMutex = { NULL };

KObjectMutex processLangemuObjectMutex;
LangemuAttributes processLangemuAttributes[0x40];

static u32 ClientSessionInfo_FindClosestSlot(KAutoObject *session)
{
    if(nbActiveSessions == 0 || session <= sessionInfos[0].session)
        return 0;
    else if(session > sessionInfos[nbActiveSessions - 1].session)
        return nbActiveSessions;

    u32 a = 0, b = nbActiveSessions - 1, m;

    do
    {
        m = (a + b) / 2;
        if(sessionInfos[m].session < session)
            a = m;
        else if(sessionInfos[m].session > session)
            b = m;
        else
            return m;
    }
    while(b - a > 1);

    return b;
}

ClientSessionInfo *ClientSessionInfo_Lookup(KAutoObject *session)
{
    u32 id = ClientSessionInfo_FindClosestSlot(session);
    return id == nbActiveSessions ? NULL : &sessionInfos[id];
}

ClientSessionInfo *ClientSessionInfo_FindFirst(const char *name)
{
    u32 i;
    for(i = 0; i < nbActiveSessions && strncmp(sessionInfos[i].name, name, 12) != 0; i++);

    return i == nbActiveSessions ? NULL : &sessionInfos[i];
}

void ClientSessionInfo_Add(KAutoObject *session, const char *name)
{
    if(nbActiveSessions == MAX_SESSION)
        return;

    KObjectMutex__Acquire(&sessionInfosMutex);

    u32 id = ClientSessionInfo_FindClosestSlot(session);

    if(id != nbActiveSessions && sessionInfos[id].session == session)
    {
        KObjectMutex__Release(&sessionInfosMutex);
        return;
    }

    for(u32 i = nbActiveSessions; i > id && i != 0; i--)
        sessionInfos[i] = sessionInfos[i - 1];

    nbActiveSessions++;

    sessionInfos[id].session = session;
    strncpy(sessionInfos[id].name, name, 12);

    ClientSessionInfo_ChangeVtable(session);
    KObjectMutex__Release(&sessionInfosMutex);
}

void ClientSessionInfo_Remove(KAutoObject *session)
{
    if(nbActiveSessions == 0)
        return;

    KObjectMutex__Acquire(&sessionInfosMutex);

    u32 id = ClientSessionInfo_FindClosestSlot(session);

    if(id == nbActiveSessions)
    {
        KObjectMutex__Release(&sessionInfosMutex);
        return;
    }

    for(u32 i = id; i < nbActiveSessions - 1; i++)
        sessionInfos[i] = sessionInfos[i + 1];

    memset(&sessionInfos[--nbActiveSessions], 0, sizeof(ClientSessionInfo));

    KObjectMutex__Release(&sessionInfosMutex);
}

static void (*KClientSession__dtor_orig)(KAutoObject *this);
static void KClientSession__dtor_hook(KAutoObject *this)
{
    KClientSession__dtor_orig(this);
    ClientSessionInfo_Remove(this);
}

void ClientSessionInfo_ChangeVtable(KAutoObject *session)
{
    static void *vtable[0x10] = { NULL }; // should be enough
    if(vtable[2] == NULL)
    {
        memcpy(vtable, session->vtable, 0x40);
        KClientSession__dtor_orig = session->vtable->dtor;
        vtable[2] = (void *)KClientSession__dtor_hook;
    }
    session->vtable = (Vtable__KAutoObject *)vtable;
}

bool doLangEmu(bool region, u32 *cmdbuf)
{
    u8 res = 0; // none
    u64 titleId = codeSetOfProcess(currentCoreContext->objectContext.currentProcess)->titleId;
    for(u32 i = 0; i < 0x40; i++)
    {
        if(processLangemuAttributes[i].titleId == titleId)
            res = region ? processLangemuAttributes[i].region : processLangemuAttributes[i].language;
    }

    if(res == 0)
        return false;

    cmdbuf[1] = 0;
    if(region)
    {
        cmdbuf[0] = (cmdbuf[0] & 0xFFFF0000) | 0x80;
        cmdbuf[2] = res - 1;
    }
    else
    {
        cmdbuf[0] = (cmdbuf[0] & 0xFFFF0000) | 0x40;
        *(u8 *)cmdbuf[4] = res - 1;
        cmdbuf[2] = cmdbuf[3] = cmdbuf[4] = 0; // just in case
    }

    return true;
}

Result doPublishToProcessHook(Handle handle, u32 *cmdbuf)
{
    Result res = 0;
    u32 pid;
    bool isPxiTerminateNotification = cmdbuf[1] == 0x100 && cmdbuf[2] == 0; // cmdbuf[2] to check for well-formed requests
    u32 cmdid = cmdbuf[0];

    if(GetProcessId(&pid, cmdbuf[3]) != 0 || pid != 4)
        isPxiTerminateNotification = false;

    res = ((Result (*)(Handle))officialSVCs[0x32])(handle);
    u32 oldrescode = cmdbuf[1];

    if(!isPxiTerminateNotification || res != 0)
        return res;

    Result res2 = 0;
    Handle rosalinaProcessHandle;
    res2 = OpenProcess(&rosalinaProcessHandle, 5);
    if(res2 != 0)
        return 0;

    cmdbuf[0] = cmdid;
    cmdbuf[1] = 0x100;
    cmdbuf[2] = 0;
    cmdbuf[3] = rosalinaProcessHandle;

    res2 = ((Result (*)(Handle))officialSVCs[0x32])(handle);

    cmdbuf[1] = oldrescode;
    (((Result (*)(Handle))officialSVCs[0x23]))(rosalinaProcessHandle);
    return 0;
}
