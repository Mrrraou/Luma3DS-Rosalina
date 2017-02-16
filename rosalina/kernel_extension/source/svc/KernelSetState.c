#include "svc/KernelSetState.h"
#include "synchronization.h"

s32 rosalinaState;

Result KernelSetStateHook(u32 type, u32 varg1, u32 varg2, u32 varg3)
{
    Result res = 0;

    switch(type)
    {
        case 0x10000:
        {
            atomicStore32(&rosalinaState, (s32) varg1);
            break;
        }
        case 0x10001:
        {
            //TODO: fix concurrency
            u32 i;
            for(i = 0; i < 0x40 && processLangemuAttributes[i].titleId != 0ULL; i++);
            if(i < 0x40)
            {
                processLangemuAttributes[i].titleId = ((u64)varg3 << 32) | (u32)varg2;
                processLangemuAttributes[i].region = (u8)(varg1 >> 8);
                processLangemuAttributes[i].language = (u8)varg1;
            }
            else
                res = 0xD8609013;
            break;
        }
        /*
        case 0x10002:
        {
            //TODO: fix concurrency
            u32 i;
            u64 titleId = ((u64)varg3 << 32) | (u32)varg2;
            for(i = 0; i < 0x40 && processLangemuAttributes[i].titleId != titleId; i++);
            if(i < 0x40)
                processLangemuAttributes[i].titleId = 0ULL;
            else
                res =  0xD88007FA;
            break;
        }*/

        default:
        {
            res = ((Result (*)(u32, u32, u32, u32))officialSVCs[0x7C])(type, varg1, varg2, varg3);
            break;
        }
    }

    return res;
}
