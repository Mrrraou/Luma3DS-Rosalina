#pragma once

#include "kernel.h"

extern KProcess * (*KProcessHandleTable__ToKProcess)(KProcessHandleTable *this, Handle processHandle);
extern KAutoObject * (*KProcessHandleTable__ToKAutoObject)(KProcessHandleTable *this, Handle handle);
extern void (*KSynchronizationObject__Signal)(KSynchronizationObject *this, bool isPulse);
extern Result (*WaitSynchronization1)(void *this_unused, KThread *thread, KSynchronizationObject *syncObject, s64 timeout);
extern Result (*KProcessHandleTable__CreateHandle)(KProcessHandleTable *this, Handle *out, KAutoObject *obj, u8 token);

extern void (*KObjectMutex__WaitAndAcquire)(KObjectMutex *this);
extern void (*KObjectMutex__ErrorOccured)(void);

extern bool (*usrToKernel8)(void *dst, const void *src, u32 len);
extern bool (*usrToKernel32)(u32 *dst, const u32 *src, u32 len);
extern bool (*usrToKernelStrncpy)(char *dst, const char *src, u32 len);
extern bool (*kernelToUsr8)(void *dst, const void *src, u32 len);
extern bool (*kernelToUsr32)(u32 *dst, const u32 *src, u32 len);
extern bool (*kernelToUsrStrncpy)(char *dst, const char *src, u32 len);

extern Result (*CustomBackdoor)(void *function, ...);

extern void (*svcFallbackHandler)(u8 svcId);
extern u32 *officialSvcHandlerTail;

extern bool isN3DS;

extern u32 *exceptionStackTop;
extern void *kernelUsrCopyFuncsStart, *kernelUsrCopyFuncsEnd;

extern bool *isDevUnit;
extern bool *enableUserExceptionHandlersForCPUExc;

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
