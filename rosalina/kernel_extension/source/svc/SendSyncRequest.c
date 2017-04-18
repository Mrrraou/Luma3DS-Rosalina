#include "svc/SendSyncRequest.h"
#include "memory.h"
#include "ipc.h"

Result SendSyncRequestHook(Handle handle)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KClientSession *clientSession = (KClientSession *)KProcessHandleTable__ToKAutoObject(handleTable, handle);

    u32 *cmdbuf = (u32 *)((u8 *)currentCoreContext->objectContext.currentThread->threadLocalStorage + 0x80);
    bool skip = false;
    Result res = 0;

    bool isValidClientSession = false;
    if(clientSession != NULL && kernelVersion >= SYSTEM_VERSION(2, 46, 0))
    {
        KClassToken tok;
        clientSession->syncObject.autoObject.vtable->GetClassToken(&tok, &clientSession->syncObject.autoObject);
        isValidClientSession = tok.flags == 0xA5;
    }
    else if(clientSession != NULL) // not the exact same test but it should work
        isValidClientSession = strcmp(clientSession->syncObject.autoObject.vtable->GetClassName(&clientSession->syncObject.autoObject), "KClientSession");

    if(isValidClientSession)
    {
        switch (cmdbuf[0])
        {
            case 0x10042:
            case 0x4010042:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && strcmp(info->name, "srv:pm") == 0)
                {
                    res = doPublishToProcessHook(handle, cmdbuf);
                    skip = true;
                }

                /*if(info != NULL)
                    info->session->autoObject.vtable->DecrementReferenceCount(&info->session->autoObject);*/
                break;
            }

            case 0x10082:
            {
                if(cmdbuf[1] != 1 || cmdbuf[2] != 0xA0002 || cmdbuf[3] != 0x1C)
                    break;

                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(false, cmdbuf);

                /*if(info != NULL)
                    info->session->autoObject.vtable->DecrementReferenceCount(&info->session->autoObject);*/

                break;
            }

            case 0x20000:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && (strcmp(info->name, "cfg:u") == 0 || strcmp(info->name, "cfg:s") == 0 || strcmp(info->name, "cfg:i") == 0))
                    skip = doLangEmu(true, cmdbuf);

                /*if(info != NULL)
                    info->session->autoObject.vtable->DecrementReferenceCount(&info->session->autoObject);*/

                break;
            }

            case 0x50100:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession);
                if(info != NULL && strcmp(info->name, "srv:") == 0)
                {
                    char name[9] = { 0 };
                    memcpy(name, cmdbuf + 1, 8);

                    skip = true;
                    res = ((Result (*)(Handle))officialSVCs[0x32])(handle);
                    if(res == 0)
                    {
                        KClientSession *outClientSession;

                        outClientSession = (KClientSession *)KProcessHandleTable__ToKAutoObject(handleTable, (Handle)cmdbuf[3]);
                        if(outClientSession != NULL)
                        {
                            SessionInfo_Add(outClientSession->parentSession, name);
                            outClientSession->syncObject.autoObject.vtable->DecrementReferenceCount(&outClientSession->syncObject.autoObject);
                        }
                    }
                }

                /*if(info != NULL)
                    info->session->autoObject.vtable->DecrementReferenceCount(&info->session->autoObject);*/

                break;
            }

            case 0x4060000:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession); // SecureInfoGetRegion
                if(info != NULL && strcmp(info->name, "cfg:s") == 0)
                    skip = doLangEmu(true, cmdbuf);

                /*if(info != NULL)
                    info->session->autoObject.vtable->DecrementReferenceCount(&info->session->autoObject);*/

                break;
            }

            case 0x8160000:
            {
                SessionInfo *info = SessionInfo_Lookup(clientSession->parentSession); // SecureInfoGetRegion
                if(info != NULL && strcmp(info->name, "cfg:i") == 0)
                    skip = doLangEmu(true, cmdbuf);

                /*if(info != NULL)
                    info->session->autoObject.vtable->DecrementReferenceCount(&info->session->autoObject);*/

                break;
            }
        }
    }

    if(clientSession != NULL)
        clientSession->syncObject.autoObject.vtable->DecrementReferenceCount(&clientSession->syncObject.autoObject);

    res = skip ? res : ((Result (*)(Handle))officialSVCs[0x32])(handle);

    return res;
}
