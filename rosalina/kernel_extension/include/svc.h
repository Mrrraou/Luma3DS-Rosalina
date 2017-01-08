#pragma once

#include "types.h"

extern void *officialSVCs[0x7E];

static inline void yield(void)
{
    ((void (*)(s64))officialSVCs[0x0A])(25*1000*1000);
}

void svcDefaultHandler(u8 svcId);
void *svcHook(u8 *pageEnd);
