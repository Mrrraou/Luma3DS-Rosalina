#include "svc/GetSystemInfo.h"
#include "utils.h"
#include "ipc.h"

Result GetSystemInfoHook(u32 dummy UNUSED, s32 type, s32 param)
{
    u64 out = 0; // should be s64 but who cares
    Result res = 0;

    switch(type)
    {
        case 0x10000:
        {
            switch(param)
            {
                case 0:
                    out = SYSTEM_VERSION(cfwInfo.versionMajor, cfwInfo.versionMinor, cfwInfo.versionBuild);
                    break;
                case 1:
                    out = cfwInfo.commitHash;
                    break;
                case 2:
                    out = cfwInfo.config;
                    break;
                case 3: // isRelease
                    out = cfwInfo.flags & 1;
                    break;
                case 4: // isN3DS
                    out = (cfwInfo.flags >> 4) & 1;
                    break;
                case 5: // isSafeMode
                    out = (cfwInfo.flags >> 5) & 1;
                    break;
                case 6: // isK9LH
                    out = (cfwInfo.flags >> 6) & 1;
                    break;
                default:
                    res = 0xF8C007F4; // not implemented
                    break;
            }
            break;
        }

        case 0x10001: // N3DS-related info
        {
            if(isN3DS)
            {
                switch(param)
                {
                    case 0:
                        out = (((PDN_MPCORE_CLKCNT >> 1) & 3) + 1) * 268;
                        break;
                    case 1:
                        out = L2C_CTRL & 1;
                        break;
                    default:
                        res = 0xF8C007F4;
                        break;
                }
            }
            else res = 0xF8C007F4;

            break;
        }

        case 0x20000: // service handles
        {
            switch(param)
            {
                case 0:
                    while(fsREGSessions[0] == NULL && fsREGSessions[1] == NULL) yield();
                    res = createHandleForThisProcess((Handle *)&out, fsREGSessions[0] == NULL ? fsREGSessions[1] : fsREGSessions[0]);
                    break;
                default:
                    res = 0xF8C007F4;
                    break;
            }
            break;
        }

        default:
            GetSystemInfo((s64 *)&out, type, param);
            break;
    }

    return setR0toR3(res, (u32)out, (u32)(out >> 32));
}
