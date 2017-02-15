#include "svc/GetCFWInfo.h"

// DEPRECATED
Result GetCFWInfo(CfwInfo *out)
{
    return kernelToUsr8(out, &cfwInfo, 16) ? 0 : 0xE0E01BF5;
}
