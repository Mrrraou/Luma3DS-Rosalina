#include "GetSystemInfo.h"

Result GetSystemInfoHook(u32 dummy, u32 type, s32 param)
{
    u32 out = 0;
    Result res = 0;

    if(type == 0x10000)
    {
        switch(param)
        {
            case 0:
                out = SYSTEM_VERSION(cfwInfo.versionMajor, cfwInfo.versionMinor, cfwInfo.versionBuild);
                break;
            case 1:
                out = cfwInfo.commitHash;
                break;
            case 2: // isRelease
                out = cfwInfo.flags & 1;
                break;
            case 3: // isK9LH
                out = (cfwInfo.flags >> 1) & 1;
                break;
            default:
                res = 0xF8C007F4; // not implemented
                break;
        }
    }

    else if(type == 0x10001) // N3DS-related info
    {
        if(isN3DS)
        {
            switch(param)
            {
                case 0:
                    out = (((PDN_MPCORE_CLKCNT >> 1) & 3) + 1) * 268;
                case 1:
                    out = L2C_CTRL & 1;
            }
        }
        else res = 0xF8C007F4;
    }
    else
        return ((Result (*) (s32, u32, s32))officialSVCs[0x2A])(dummy, type, param);

    return setR0toR3(res, out, 0);
}
