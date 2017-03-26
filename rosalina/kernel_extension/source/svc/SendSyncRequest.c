#include "svc/SendSyncRequest.h"
#include "memory.h"
#include "ipc.h"

Result SendSyncRequestHook(Handle handle)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, handle);

    TracedService *traced = NULL;
    u32 *cmdbuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x80);
    bool skip = false;
    Result res = 0;

    if(session != NULL)
    {
        switch (cmdbuf[0])
        {
            case 0x10042:
            case 0x4010042:
            {
                u32 index = TracedService_Lookup(&srvPmService, session);
                if(index < 2)
                {
                    res = doPublishToProcessHook(handle, cmdbuf);
                    skip = true;
                }

                break;
            }

            case 0x10082:
            {
                if(cmdbuf[1] != 1 || cmdbuf[2] != 0xA0002 || cmdbuf[3] != 0x1C) break;

                u32 index = TracedService_Lookup(&cfgUService, session); // GetConfigInfoBlk2
                if(index >= 25) index = TracedService_Lookup(&cfgSService, session);
                if(index >= 25) index = TracedService_Lookup(&cfgIService, session);
                if(index < 25)
                    skip = doLangEmu(false, cmdbuf);

                break;
            }

            case 0x20000:
            {
                u32 index = TracedService_Lookup(&cfgUService, session); // SecureInfoGetRegion
                if(index >= 25) index = TracedService_Lookup(&cfgSService, session);
                if(index >= 25) index = TracedService_Lookup(&cfgIService, session);
                if(index < 25)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }

            case 0x50100:
            {
                u32 index = TracedService_Lookup(&srvPort, session); // GetServiceHandle
                if(index < 0x40)
                {
                    const char *serviceName = (const char *)(cmdbuf + 1);
                    for(u32 i = 0; i < sizeof(tracedServices) / sizeof(TracedService *); i++)
                    {
                        if(strncmp(serviceName, tracedServices[i]->name, 8) == 0)
                            traced = tracedServices[i];
                    }
                }
                break;
            }

            case 0x4060000:
            {
                u32 index = TracedService_Lookup(&cfgSService, session); // SecureInfoGetRegion
                if(index < 25)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }

            case 0x8160000:
            {
                u32 index = TracedService_Lookup(&cfgIService, session); // SecureInfoGetRegion
                if(index < 25)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }
        }

        session->vtable->DecrementReferenceCount(session);
    }

    res = skip ? res : ((Result (*)(Handle))officialSVCs[0x32])(handle);
    if(res == 0 && traced != NULL)
    {
        session = KProcessHandleTable__ToKAutoObject(handleTable, (Handle)cmdbuf[3]);
        if(session != NULL)
        {
            TracedService_Add(traced, session);
            session->vtable->DecrementReferenceCount(session);
        }
    }

    return res;
}
