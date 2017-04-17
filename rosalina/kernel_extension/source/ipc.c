#include "ipc.h"
#include "memory.h"

static SessionInfo sessionInfos[MAX_SESSION] = { {NULL} };
static u32 nbActiveSessions = 0;
static KRecursiveLock sessionInfosLock = { NULL };

KRecursiveLock processLangemuLock;
LangemuAttributes processLangemuAttributes[0x40];

static void *customSessionVtable[0x10] = { NULL }; // should be enough

static u32 SessionInfo_FindClosestSlot(KSession *session)
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

SessionInfo *SessionInfo_Lookup(KSession *session)
{
    KRecursiveLock__Lock(criticalSectionLock);
    KRecursiveLock__Lock(&sessionInfosLock);

    SessionInfo *ret;
    u32 id = SessionInfo_FindClosestSlot(session);
    if(id == nbActiveSessions)
        ret = NULL;
    else
    {
        ret = (void **)(sessionInfos[id].session->autoObject.vtable) == customSessionVtable ? &sessionInfos[id] : NULL;
        /*if(ret != NULL)
            KAutoObject__AddReference(&ret->session->autoObject);*/
    }
    KRecursiveLock__Unlock(&sessionInfosLock);
    KRecursiveLock__Unlock(criticalSectionLock);

    return ret;
}

SessionInfo *SessionInfo_FindFirst(const char *name)
{
    KRecursiveLock__Lock(criticalSectionLock);
    KRecursiveLock__Lock(&sessionInfosLock);

    SessionInfo *ret;
    u32 id;
    for(id = 0; id < nbActiveSessions && strncmp(sessionInfos[id].name, name, 12) != 0; id++);
    if(id == nbActiveSessions)
        ret = NULL;
    else
    {
        ret = (void **)(sessionInfos[id].session->autoObject.vtable) == customSessionVtable ? &sessionInfos[id] : NULL;
        /*if(ret != NULL)
            KAutoObject__AddReference(&ret->session->autoObject);*/
    }

    KRecursiveLock__Unlock(&sessionInfosLock);
    KRecursiveLock__Unlock(criticalSectionLock);

    return ret;
}

void SessionInfo_Add(KSession *session, const char *name)
{
    KAutoObject__AddReference(&session->autoObject);
    SessionInfo_ChangeVtable(session);
    session->autoObject.vtable->DecrementReferenceCount(&session->autoObject);

    KRecursiveLock__Lock(criticalSectionLock);
    KRecursiveLock__Lock(&sessionInfosLock);

    if(nbActiveSessions == MAX_SESSION)
    {
        KRecursiveLock__Unlock(&sessionInfosLock);
        KRecursiveLock__Unlock(criticalSectionLock);
        return;
    }

    u32 id = SessionInfo_FindClosestSlot(session);

    if(id != nbActiveSessions && sessionInfos[id].session == session)
    {
        KRecursiveLock__Unlock(&sessionInfosLock);
        KRecursiveLock__Unlock(criticalSectionLock);
        return;
    }

    for(u32 i = nbActiveSessions; i > id && i != 0; i--)
        sessionInfos[i] = sessionInfos[i - 1];

    nbActiveSessions++;

    sessionInfos[id].session = session;
    strncpy(sessionInfos[id].name, name, 12);

    KRecursiveLock__Unlock(&sessionInfosLock);
    KRecursiveLock__Unlock(criticalSectionLock);
}

void SessionInfo_Remove(KSession *session)
{
    KRecursiveLock__Lock(criticalSectionLock);
    KRecursiveLock__Lock(&sessionInfosLock);

    if(nbActiveSessions == MAX_SESSION)
    {
        KRecursiveLock__Unlock(&sessionInfosLock);
        KRecursiveLock__Unlock(criticalSectionLock);
        return;
    }

    u32 id = SessionInfo_FindClosestSlot(session);

    if(id == nbActiveSessions)
    {
        KRecursiveLock__Unlock(&sessionInfosLock);
        KRecursiveLock__Unlock(criticalSectionLock);
        return;
    }

    for(u32 i = id; i < nbActiveSessions - 1; i++)
        sessionInfos[i] = sessionInfos[i + 1];

    memset(&sessionInfos[--nbActiveSessions], 0, sizeof(SessionInfo));

    KRecursiveLock__Unlock(&sessionInfosLock);
    KRecursiveLock__Unlock(criticalSectionLock);
}

static void (*KSession__dtor_orig)(KAutoObject *this);
static void KSession__dtor_hook(KAutoObject *this)
{
    KSession__dtor_orig(this);
    SessionInfo_Remove((KSession *)this);
}

void SessionInfo_ChangeVtable(KSession *session)
{
    if(customSessionVtable[2] == NULL)
    {
        memcpy(customSessionVtable, session->autoObject.vtable, 0x40);
        KSession__dtor_orig = session->autoObject.vtable->dtor;
        customSessionVtable[2] = (void *)KSession__dtor_hook;
    }
    session->autoObject.vtable = (Vtable__KAutoObject *)customSessionVtable;
}

bool doLangEmu(bool region, u32 *cmdbuf)
{
    KRecursiveLock__Lock(criticalSectionLock);
    KRecursiveLock__Lock(&processLangemuLock);

    u8 res = 0; // none
    u64 titleId = codeSetOfProcess(currentCoreContext->objectContext.currentProcess)->titleId;
    for(u32 i = 0; i < 0x40; i++)
    {
        if(processLangemuAttributes[i].titleId == titleId)
            res = region ? processLangemuAttributes[i].region : processLangemuAttributes[i].language;
    }

    if(res == 0)
    {
        KRecursiveLock__Unlock(&processLangemuLock);
        KRecursiveLock__Unlock(criticalSectionLock);
        return false;
    }

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

    KRecursiveLock__Unlock(&processLangemuLock);
    KRecursiveLock__Unlock(criticalSectionLock);
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
