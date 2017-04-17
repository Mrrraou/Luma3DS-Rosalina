#include "svc/ConnectToPort.h"
#include "memory.h"
#include "ipc.h"

Result ConnectToPortHook(Handle *out, const char *name)
{
    char portName[12] = {0};
    Result res = 0;
    if(name != NULL)
    {
        s32 nb = usrToKernelStrncpy(portName, name, 12);
        if(nb < 0)
            return 0xD9001814;
        else if(nb == 12 && portName[11] != 0)
            return 0xE0E0181E;
    }
    res = ConnectToPort(out, name);
    if(res != 0)
        return res;

    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KClientSession *clientSession = (KClientSession *)KProcessHandleTable__ToKAutoObject(handleTable, *out);
    if(clientSession != NULL)
    {
        SessionInfo_Add(clientSession->parentSession, portName);
        clientSession->syncObject.autoObject.vtable->DecrementReferenceCount(&clientSession->syncObject.autoObject);
    }

    return res;
}
