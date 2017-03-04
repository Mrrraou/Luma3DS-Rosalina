#pragma once

#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/result.h>

// For accessing physmem uncached (and directly)
#define PA_PTR(addr)            (void *)((u32)(addr) | 1 << 31)
#define PA_FROM_VA_PTR(addr)    PA_PTR(convertVAToPA(addr))
#define REG32(addr)             (*(vu32 *)(PA_PTR(addr)))

static inline void *decodeARMBranch(const void *src)
{
    u32 instr = *(const u32 *)src;
    s32 off = (instr & 0xFFFFFF) << 2;
    off = (off << 6) >> 6; // sign extend

    return (void *)((const u8 *)src + 8 + off);
}

Result svc0x2F(void *function, ...); // custom backdoor before kernel ext. is installed (and only before!)

void *convertVAToPA(const void *VA);

u32 getNumberOfCores(void);

extern u8 kernel_extension[];
extern u32 kernel_extension_size;
