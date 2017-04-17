#include "svc/SendSyncRequest.h"
#include "memory.h"
#include "ipc.h"

Result SendSyncRequestHook(Handle handle)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KAutoObject *session = KProcessHandleTable__ToKAutoObject(handleTable, handle);

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
                ClientSessionInfo *info = ClientSessionInfo_Lookup(session);
                if(info != NULL && strcmp(info->name, "srv:pm") == 0)
                {
                    res = doPublishToProcessHook(handle, cmdbuf);
                    skip = true;
                }

                break;
            }

            case 0x10082:
            {
                if(cmdbuf[1] != 1 || cmdbuf[2] != 0xA0002 || cmdbuf[3] != 0x1C)
                    break;

                ClientSessionInfo *info = ClientSessionInfo_Lookup(session);
                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(false, cmdbuf);

                break;
            }

            case 0x20000:
            {
                ClientSessionInfo *info = ClientSessionInfo_Lookup(session);
                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(true, cmdbuf);

                break;
            }

            case 0x50100:
            {
                ClientSessionInfo *info = ClientSessionInfo_Lookup(session);
                if(info != NULL && strcmp(info->name, "srv:") == 0)
                {
                    char name[9] = { 0 };
                    memcpy(name, cmdbuf + 1, 8);

                    skip = true;
                    res = ((Result (*)(Handle))officialSVCs[0x32])(handle);
                    if(res == 0)
                    {
                        KAutoObject *outSession;

                        outSession = KProcessHandleTable__ToKAutoObject(handleTable, (Handle)cmdbuf[3]);
                        if(outSession != NULL)
                        {
                            ClientSessionInfo_Add(outSession, name);
                            session->vtable->DecrementReferenceCount(outSession);
                        }
                    }
                }

                break;
            }

            case 0x4060000:
            {
                ClientSessionInfo *info = ClientSessionInfo_Lookup(session); // SecureInfoGetRegion
                if(info != NULL && strcmp(info->name, "cfg:s") == 0)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }

            case 0x8160000:
            {
                ClientSessionInfo *info = ClientSessionInfo_Lookup(session); // SecureInfoGetRegion
                if(info != NULL && strcmp(info->name, "cfg:i") == 0)
                    skip = doLangEmu(true, cmdbuf);

                break;
            }
        }

        session->vtable->DecrementReferenceCount(session);
    }

    res = skip ? res : ((Result (*)(Handle))officialSVCs[0x32])(handle);

    return res;
}
