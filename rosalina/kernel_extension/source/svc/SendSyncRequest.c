#include "svc/SendSyncRequest.h"
#include "memory.h"

static inline bool doLangEmu(bool region, u32 *cmdbuf)
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

Result SendSyncRequestHook(Handle handle)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, handle);

    TracedService *traced = NULL;
    u32 *cmdbuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x80);
    bool skip = false;

    if(session != NULL)
    {
        session->vtable->DecrementReferenceCount(session);

        switch (cmdbuf[0])
        {
            case 0x10082:
            {
                if(cmdbuf[1] != 1 || cmdbuf[2] != 0xA0002 || cmdbuf[3] != 0x1C) break;

                u32 index = lookUpInSessionArray(session, cfgUSessions, 25); // GetConfigInfoBlk2
                if(index >= 25) index = lookUpInSessionArray(session, cfgSSessions, 25);
                if(index >= 25) index = lookUpInSessionArray(session, cfgISessions, 25);
                if(index < 25)
                    skip = doLangEmu(false, cmdbuf);

                break;
            }

            case 0x20000:
            {
                u32 index = lookUpInSessionArray(session, cfgUSessions, 25); // SecureInfoGetRegion
                if(index >= 25) index = lookUpInSessionArray(session, cfgSSessions, 25);
                if(index >= 25) index = lookUpInSessionArray(session, cfgISessions, 25);
                if(index < 25)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }

            case 0x50100:
            {
                u32 index = lookUpInSessionArray(session, srvSessions, 0x40); // GetServiceHandle
                if(index < 0x40)
                {
                    const char *serviceName = (const char *)(cmdbuf + 1);
                    for(u32 i = 0; i < sizeof(tracedServices) / sizeof(TracedService); i++)
                    {
                        if(strncmp(serviceName, tracedServices[i].name, 8) == 0)
                            traced = tracedServices + i;
                    }
                }
                break;
            }

            case 0x4060000:
            {
                u32 index = lookUpInSessionArray(session, cfgSSessions, 25); // SecureInfoGetRegion
                if(index < 25)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }

            case 0x8160000:
            {
                u32 index = lookUpInSessionArray(session, cfgISessions, 25); // SecureInfoGetRegion
                if(index < 25)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }

        }

    }

    Result res = skip ? 0 : ((Result (*)(Handle))officialSVCs[0x32])(handle);
    if(traced != NULL)
    {
        session = KProcessHandleTable__ToKAutoObject(handleTable, (Handle)cmdbuf[3]);
        if(session != NULL)
        {
            session->vtable->DecrementReferenceCount(session);
            addToSessionArray(session, traced->sessions, traced->maxSessions);
        }
    }

    return res;
}
