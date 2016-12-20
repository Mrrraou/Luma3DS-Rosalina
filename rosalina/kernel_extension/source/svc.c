#include "svc.h"
#include "svc/Break.h"

SupervisorCallOverride overrideList[] =
{
    {0x3C, BreakHook}
};

u32 overrideListSize = sizeof(overrideList) / sizeof(SupervisorCallOverride);
