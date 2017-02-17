#include "ipc.h"
#include "memory.h"

KAutoObject *srvSessions[0x40] = {NULL};
TracedService srvPort = {"srv:", srvSessions, 0x40, {NULL}};

KAutoObject *fsREGSessions[2] = {NULL};
KAutoObject *cfgUSessions[25] = {NULL}, *cfgSSessions[25] = {NULL}, *cfgISessions[25] = {NULL};

TracedService   fsREGService = {"fs:REG", fsREGSessions, 2, {NULL}},
                cfgUService  = {"cfg:u", cfgUSessions,  25, {NULL}},
                cfgSService  = {"cfg:s", cfgSSessions,  25, {NULL}},
                cfgIService  = {"cfg:i", cfgISessions,  25, {NULL}};

TracedService *tracedServices[4] =
{
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
    for(u32 i = 0; !res && i < sizeof(tracedServices)/sizeof(TracedService *) && !TracedService_Remove(tracedServices[i], this); i++);
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

    if(res == 0) return false;

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
