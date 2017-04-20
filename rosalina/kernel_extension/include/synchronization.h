#pragma once

#include "types.h"
#include "kernel.h"

typedef KSchedulableInterruptEvent* (*SGI0Handler_t)(KBaseInterruptEvent *this, u32 interruptID);

// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/CCHDIFIJ.html
void executeFunctionOnCores(SGI0Handler_t func, u8 targetList, u8 targetListFilter);

u32 getNumberOfCores(void);

// Taken from ctrulib:

static inline void __dsb(void)
{
	__asm__ __volatile__("mcr p15, 0, %[val], c7, c10, 4" :: [val] "r" (0) : "memory");
}

static inline void __clrex(void)
{
    __asm__ __volatile__("clrex" ::: "memory");
}

static inline s32 __ldrex(s32* addr)
{
    s32 val;
    __asm__ __volatile__("ldrex %[val], %[addr]" : [val] "=r" (val) : [addr] "Q" (*addr));
    return val;
}

static inline bool __strex(s32* addr, s32 val)
{
    bool res;
    __asm__ __volatile__("strex %[res], %[val], %[addr]" : [res] "=&r" (res) : [val] "r" (val), [addr] "Q" (*addr));
    return res;
}
