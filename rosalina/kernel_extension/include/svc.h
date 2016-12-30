#pragma once

#include "types.h"

void *officialSVCs[0x7E] = {NULL};

static inline void yield(void)
{
    ((void (*)(s64))officialSVCs[0x0A])(0);
}

void svcDefaultHandler(u8 svcId);
void *svcHook(u32 regs, const u32 *userRegs);
