#include "ipc.h"
#include "memory.h"

KAutoObject *srvSessions[0x40] = {NULL};
TracedService srvPort = {"srv:", srvSessions, 0x40, {NULL}};

KAutoObject *srvPmSessions[2] = {NULL};
KAutoObject *fsREGSessions[2] = {NULL};
KAutoObject *cfgUSessions[25] = {NULL}, *cfgSSessions[25] = {NULL}, *cfgISessions[25] = {NULL};

TracedService   srvPmService = {"srv:pm", srvPmSessions, 2, {NULL}},
                fsREGService = {"fs:REG", fsREGSessions, 2, {NULL}},
                cfgUService  = {"cfg:u", cfgUSessions,  25, {NULL}},
                cfgSService  = {"cfg:s", cfgSSessions,  25, {NULL}},
                cfgIService  = {"cfg:i", cfgISessions,  25, {NULL}};

TracedService *tracedServices[5] =
{
    &srvPmService, // it's a port on < 7.x
    &fsREGService,
    &cfgUService,
    &cfgSService,
    &cfgIService
};

KObjectMutex processLangemuObjectMutex;
LangemuAttributes processLangemuAttributes[0x40] = {{0ULL}};

static void (*KClientSession__dtor_orig)(KAutoObject *this);
static void KClientSession__dtor_hook(KAutoObject *this)
{
    bool res = TracedService_Remove(&srvPort, this);
    for(u32 i = 0; !res && i < sizeof(tracedServices) / sizeof(TracedService *) && !TracedService_Remove(tracedServices[i], this); i++);
    KClientSession__dtor_orig(this);
}

void TracedService_ChangeClientSessionVtable(KAutoObject *obj)
{
    static void *vtable[0x10] = {NULL}; // should be enough
    if(vtable[2] == NULL)
    {
        memcpy(vtable, obj->vtable, 0x40);
        KClientSession__dtor_orig = obj->vtable->dtor;
        vtable[2] = (void *)KClientSession__dtor_hook;
    }
    obj->vtable = (Vtable__KAutoObject *)vtable;
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
