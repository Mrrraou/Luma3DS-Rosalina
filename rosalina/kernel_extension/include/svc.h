#pragma once

#include "types.h"

typedef struct SupervisorCallOverride
{
    u32 index;
    void *func;
} SupervisorCallOverride;

extern SupervisorCallOverride overrideList[];
extern u32 overrideListSize;
