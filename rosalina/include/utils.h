#pragma once

#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/result.h>

// For accessing physmem uncached (and directly)
#define PA_PTR(addr)            (void *)((u32)(addr) | 1 << 31)
#define PA_FROM_VA_PTR(addr)    PA_PTR(convertVAToPA(addr))

static inline void assertSuccess(Result res)
{
    if(R_FAILED(res)) svcBreak(USERBREAK_PANIC);
}

typedef u32(*backdoor_fn)(u32 arg0, u32 arg1);
u32 svc_7b(void* entry_fn, ...); // can pass up to two arguments to entry_fn(...)

void *convertVAToPA(const void *VA);

u32 *getTTB1Address(void);

void dsb(void);

extern const u8 kernel_extension[];
extern const u32 kernel_extension_size;
