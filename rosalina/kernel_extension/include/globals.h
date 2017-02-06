#pragma once

#include "kernel.h"

extern KProcess * (*KProcessHandleTable__ToKProcess)(KProcessHandleTable *this, Handle processHandle);
extern KAutoObject * (*KProcessHandleTable__ToKAutoObject)(KProcessHandleTable *this, Handle handle);
extern void (*KSynchronizationObject__Signal)(KSynchronizationObject *this, bool isPulse);
extern Result (*WaitSynchronization1)(void *this_unused, KThread *thread, KSynchronizationObject *syncObject, s64 timeout);

extern void (*svcFallbackHandler)(u8 svcId);
extern u32 *officialSvcHandlerTail;

extern bool isN3DS;

extern u32 *exceptionStackTop;
extern void *kernelUsrCopyFuncsStart, *kernelUsrCopyFuncsEnd;

bool *isDevUnit;
bool *enableUserExceptionHandlersForCPUExc;

extern InterruptManager *interruptManager;
extern KBaseInterruptEvent *customInterruptEvent;

extern void (*flushEntireICache)(void);
extern void (*flushEntireDCacheAndL2C)(void); // reentrant, but unsafe in fatal exception contexts in case the kernel is f*cked up
extern void (*initFPU)(void);
extern void (*mcuReboot)(void);
extern void (*coreBarrier)(void);

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
