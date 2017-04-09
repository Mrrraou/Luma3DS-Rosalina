#pragma once

#include "kernel.h"

extern KProcess * (*KProcessHandleTable__ToKProcess)(KProcessHandleTable *this, Handle processHandle);
extern KThread * (*KProcessHandleTable__ToKThread)(KProcessHandleTable *this, Handle threadHandle);
extern KAutoObject * (*KProcessHandleTable__ToKAutoObject)(KProcessHandleTable *this, Handle handle);
extern void (*KSynchronizationObject__Signal)(KSynchronizationObject *this, bool isPulse);
extern Result (*WaitSynchronization1)(void *this_unused, KThread *thread, KSynchronizationObject *syncObject, s64 timeout);
extern Result (*KProcessHandleTable__CreateHandle)(KProcessHandleTable *this, Handle *out, KAutoObject *obj, u8 token);
extern Result (*KProcessHwInfo__MapProcessMemory)(KProcessHwInfo *this, KProcessHwInfo *other, void *dst, void *src, u32 nbPages);
extern void (*KObjectMutex__WaitAndAcquire)(KObjectMutex *this);
extern void (*KObjectMutex__ErrorOccured)(void);

extern Result (*GetSystemInfo)(s64 *out, s32 type, s32 param);
extern Result (*GetProcessInfo)(s64 *out, Handle processHandle, u32 type);
extern Result (*GetThreadInfo)(s64 *out, Handle threadHandle, u32 type);
extern Result (*ConnectToPort)(Handle *out, const char *name);
extern Result (*OpenProcess)(Handle *out, u32 processId);
extern Result (*GetProcessId)(u32 *out, Handle process);
extern Result (*DebugActiveProcess)(Handle *out, u32 processId);

extern void (*KTimerAndWDTManager__Sanitize)(KTimerAndWDTManager *this);

extern void (*flushDataCacheRange)(void *addr, u32 len);
extern void (*invalidateInstructionCacheRange)(void *addr, u32 len);

extern bool (*usrToKernelMemcpy8)(void *dst, const void *src, u32 len);
extern bool (*usrToKernelMemcpy32)(u32 *dst, const u32 *src, u32 len);
extern s32 (*usrToKernelStrncpy)(char *dst, const char *src, u32 len);
extern bool (*kernelToUsrMemcpy8)(void *dst, const void *src, u32 len);
extern bool (*kernelToUsrMemcpy32)(u32 *dst, const u32 *src, u32 len);
extern s32 (*kernelToUsrStrncpy)(char *dst, const char *src, u32 len);

extern Result (*CustomBackdoor)(void *function, ...);

extern void (*svcFallbackHandler)(u8 svcId);
extern void (*kernelpanic)(void);
extern u32 *officialSvcHandlerTail;

extern Result (*SignalDebugEvent)(DebugEventType type, u32 info, ...);

extern bool isN3DS;

extern u32 *exceptionStackTop;
extern void *kernelUsrCopyFuncsStart, *kernelUsrCopyFuncsEnd;

extern bool *isDevUnit;
extern bool *enableUserExceptionHandlersForCPUExc;

extern InterruptManager *interruptManager;
extern KBaseInterruptEvent *customInterruptEvent;

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
