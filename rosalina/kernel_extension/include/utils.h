#pragma once

#include "types.h"
#include "kernel.h"

// For accessing physmem uncached (and directly)
#define PA_PTR(addr)            (void *)((u32)(addr) | 1u << 31)
#define PA_FROM_VA_PTR(addr)    PA_PTR(convertVAToPA(addr, false))

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

// For ARM prologs in the form of: push {regs} ... sub sp, #off (this obviously doesn't intend to cover all cases)
static inline u32 computeARMFrameSize(const u32 *prolog)
{
    const u32 *off;

    for(off = prolog; (*off >> 16) != 0xE92D; off++); // look for stmfd sp! = push
    u32 nbPushedRegs = 0;
    for(u32 val = *off & 0xFFFF; val != 0; val >>= 1) // 1 bit = 1 pushed register
        nbPushedRegs += val & 1;
    for(; (*off >> 8) != 0xE24DD0; off++); // look for sub sp, #offset
    u32 localVariablesSpaceSize = *off & 0xFF;

    return 4 * nbPushedRegs + localVariablesSpaceSize;
}

static inline u32 getCurrentCoreID(void)
{
    u32 coreId;
    __asm__ volatile("mrc p15, 0, %0, c0, c0, 5" : "=r"(coreId));
    return coreId & 3;
}

u32 convertVAToPA(const void *addr, bool writeCheck);

u32 safecpy(void *dst, const void *src, u32 len);
void KObjectMutex__Acquire(KObjectMutex *this);
void KObjectMutex__Release(KObjectMutex *this);

void flushEntireDataCache(void);
void invalidateEntireInstructionCache(void);
