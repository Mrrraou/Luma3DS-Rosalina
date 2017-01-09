#pragma once

#include "types.h"
#include "kernel.h"

// For accessing physmem uncached (and directly)
#define PA_PTR(addr)            (void *)((u32)(addr) | 1u << 31)
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
u32 getCurrentCoreID(void);
bool enableIRQ(void);

KProcess * (*KProcessHandleTable__ToKProcess)(KProcessHandleTable *this, Handle processHandle);
KAutoObject * (*KProcessHandleTable__ToKAutoObject)(KProcessHandleTable *this, Handle handle);
void (*KSynchronizationObject__Signal)(KSynchronizationObject *this, bool isPulse);
Result (*WaitSynchronization1)(void *this_unused, KThread *thread, KSynchronizationObject *syncObject, s64 timeout);


void (*svcFallbackHandler)(u8 svcId);
u32 *officialSvcHandlerTail;

extern InterruptManager *interruptManager;
extern KBaseInterruptEvent *customInterruptEvent;

extern void (*flushEntireICache)(void);
extern void (*flushEntireDCacheAndL2C)(void); // reentrant, but unsafe in fatal exception contexts in case the kernel is f*cked up
extern void (*initFPU)(void);
extern void (*mcuReboot)(void);
extern void (*coreBarrier)(void);

Result setR0toR3(Result r0, ...);

extern bool isN3DS;

typedef struct PACKED CfwInfo
{
    char magic[4];

    u8 versionMajor;
    u8 versionMinor;
    u8 versionBuild;
    u8 flags;

    u32 commitHash;

    u32 config;
} CfwInfo;

extern CfwInfo cfwInfo;

void atomicStore32(s32 *dst, s32 value);
