#include "KernelSetState.h"
#include "../synchronization.h"

Result KernelSetStateHook(u32 type, u32 varg1, u32 varg2, u32 varg3, u32 varg4)
{
    if(type == 0x10000)
    {
        do
            __ldrex(&rosalinaState);
        while(__strex(&rosalinaState, (s32) varg1));
        return 0;
    }
    else
        ((Result (*)(u32, u32, u32, u32, u32))officialSVCs[0x7C])(type, varg1, varg2, varg3, varg4);
}
