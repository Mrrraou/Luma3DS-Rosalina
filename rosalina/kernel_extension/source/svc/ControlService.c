#include "svc/ControlService.h"
#include "memory.h"
#include "ipc.h"

Result ControlService(ServiceOp op, u32 varg1, u32 varg2)
{
    Result res = 0;
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);

    switch(op)
    {
        case SERVICEOP_GET_NAME:
        {
            KSession *session = NULL;
            SessionInfo *info = NULL;
            KAutoObject *obj = KProcessHandleTable__ToKAutoObject(handleTable, (Handle)varg2);
            if(obj == NULL)
                return 0xD8E007F7; // invalid handle
            else if(kernelVersion >= SYSTEM_VERSION(2, 46, 0))
            {
                KClassToken tok;
                obj->vtable->GetClassToken(&tok, obj);
                if(tok.flags == 0x95)
                    session = ((KServerSession *)obj)->parentSession;
                else if(tok.flags == 0xA5)
                    session = ((KClientSession *)obj)->parentSession;
            }
            else
            {   // not the exact same tests but it should work
                if(strcmp(obj->vtable->GetClassName(obj), "KServerSession") == 0)
                    session = ((KServerSession *)obj)->parentSession;
                else if(strcmp(obj->vtable->GetClassName(obj), "KClientSession") == 0)
                    session = ((KClientSession *)obj)->parentSession;
            }

            if(session != NULL)
                info = SessionInfo_Lookup(session);

            if(info == NULL)
                res = 0xD8E007F7;
            else
            {
                // names are limited to 11 characters (for ports)
                // kernelToUsrStrncpy doesn't clear trailing bytes
                char name[12] = { 0 };
                strncpy(name, info->name, 12);
                res = kernelToUsrMemcpy8((void *)varg1, name, strlen(name) + 1) ? 0 : 0xD9001814;
            }

            obj->vtable->DecrementReferenceCount(obj);
            return res;
        }

        case SERVICEOP_STEAL_CLIENT_SESSION:
        {
            char name[12] = { 0 };
            SessionInfo *info = NULL;
            if(name != NULL)
            {
                s32 nb = usrToKernelStrncpy(name, (const char *)varg2, 12);
                if(nb < 0)
                    return 0xD9001814;
                else if(nb == 12 && name[11] != 0)
                    return 0xE0E0181E;
            }

            info = SessionInfo_FindFirst(name);

            if(info == NULL)
                return 0x9401BFE; // timeout (the wanted service is likely not initalized)
            else
            {
                Handle out;
                res = createHandleForThisProcess(&out, &info->session->clientSession.syncObject.autoObject);
                return (res != 0) ? res : (kernelToUsrMemcpy32((u32 *)varg1, (u32 *)&out, 4) ? 0 : (Result)0xE0E01BF5);
            }
        }

        default:
            return 0xF8C007F4;
    }
}
