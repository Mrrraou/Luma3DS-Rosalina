#pragma once

#include "types.h"
#include "kernel.h"

// For accessing physmem uncached (and directly)
#define PA_PTR(addr)            (void *)((u32)(addr) | 1 << 31)
#define PA_FROM_VA_PTR(addr)    PA_PTR(convertVAToPA(addr))

static inline u32 makeARMBranch(const void *src, const void *dst, bool link) // the macros for those are ugly and buggy
{
    u32 instrBase = link ? 0xEB000000 : 0xEA000000;
    u32 off = (u32)((const u8 *)dst - ((const u8 *)src + 8)); // the PC is always two instructions ahead of the one being executed

    return instrBase | ((off >> 2) & 0xFFFFFF);
}

static inline void *decodeARMBranch(const void *src)
{
    u32 instr = *(const u32 *)src;
    s32 off = (instr & 0xFFFFFF) << 2;
    off = (off << 6) >> 6; // sign extend

    return (void *)((const u8 *)src + 8 + off);
}

void *convertVAToPA(const void *addr);
u32 getNumberOfCores(void);

extern InterruptEvent *customInterruptEvent;
typedef void (*SGI0Callback_t)(void);

// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/CCHDIFIJ.html
void executeFunctionOnCores(SGI0Callback_t func, u32 targetList, u32 targetListFilter);
