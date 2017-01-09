#include "svc/KernelSetState.h"
#include "synchronization.h"

s32 rosalinaState;

Result KernelSetStateHook(u32 type, u32 varg1, u32 varg2, u32 varg3, u32 varg4)
{
    if(type == 0x10000)
    {
        atomicStore32(&rosalinaState, (s32) varg1);
        return 0;
    }
/*

if(type == 0x10000)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KSynchronizationObject *obj = NULL;
    KClassToken token = {0};

    if(varg1 != 0)
        obj = (KSynchronizationObject *)KProcessHandleTable__ToKAutoObject(handleTable, (Handle) varg1);

    if(obj == NULL && varg1 != 0)
        return 0xD8E007F7;

    if(obj != NULL)
    {
        obj->autoObject.vtable->GetClassToken(&token, (KAutoObject *)obj);
        if((token.flags & 1) == 0)
        {
            obj->autoObject.vtable->DecrementReferenceCount((KAutoObject *)obj);
            return 0xD8E007F7;
        }
    }
    // A bit unsecure but we don't really care... :

    if(rosalinaSyncObj != NULL)
    {
        KEvent *ev = (KEvent *)rosalinaSyncObj;
        KTimer *tmr = (KTimer *)rosalinaSyncObj;
        KSynchronizationObject__Signal(rosalinaSyncObj, token.flags == 0x1F ? ev->resetType == RESET_PULSE : (token.flags == 0x35 ? tmr->resetType == RESET_PULSE : false));
        rosalinaSyncObj->autoObject.vtable->DecrementReferenceCount((KAutoObject *)rosalinaSyncObj);
    }

    atomicStore32((s32 *)&rosalinaSyncObj, (s32) obj);
    return 0;
}
*/
    else
        return ((Result (*)(u32, u32, u32, u32, u32))officialSVCs[0x7C])(type, varg1, varg2, varg3, varg4);
}
