#include "svc/TranslateHandle.h"
#include "memory.h"

Result TranslateHandle(u32 *outKAddr, char *outClassName, Handle handle)
{
    KProcessHandleTable *handleTable = handleTableOfProcess(currentCoreContext->objectContext.currentProcess);
    KAutoObject *obj;
    Result res;
    const char *name;

    if(handle == CUR_THREAD_HANDLE)
    {
        obj = (KAutoObject *)currentCoreContext->objectContext.currentProcess;
        KAutoObject__AddReference(obj);
    }
    else if(handle == CUR_PROCESS_HANDLE)
    {
        obj = (KAutoObject *)currentCoreContext->objectContext.currentProcess;
        KAutoObject__AddReference(obj);
    }
    else
        obj = KProcessHandleTable__ToKAutoObject(handleTable, handle);

    if(obj == NULL)
        return 0xD8E007F7; // invalid handle

    if(kernelVersion >= SYSTEM_VERSION(2, 46, 0))
    {
        KClassToken tok;
        obj->vtable->GetClassToken(&tok, obj);
        name = tok.name;
    }
    else
        name = obj->vtable->GetClassName(obj);

    if(name == NULL) // shouldn't happen
        name = "KAutoObject";

    *outKAddr = (u32)obj;
    res = kernelToUsrMemcpy8(outClassName, name, strlen(name) + 1) ? 0 : 0xE0E01BF5;

    obj->vtable->DecrementReferenceCount(obj);
    return res;
}
