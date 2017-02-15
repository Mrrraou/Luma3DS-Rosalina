#include "kernel.h"

KProcess * (*KProcessHandleTable__ToKProcess)(KProcessHandleTable *this, Handle processHandle);
KAutoObject * (*KProcessHandleTable__ToKAutoObject)(KProcessHandleTable *this, Handle handle);
void (*KSynchronizationObject__Signal)(KSynchronizationObject *this, bool isPulse);
Result (*WaitSynchronization1)(void *this_unused, KThread *thread, KSynchronizationObject *syncObject, s64 timeout);
Result (*KProcessHandleTable__CreateHandle)(KProcessHandleTable *this, Handle *out, KAutoObject *obj, u8 token);
bool (*usrStrncpy)(char *dst, const char *src, u32 len);

bool (*usrToKernel8)(void *dst, const void *src, u32 len);
bool (*usrToKernel32)(u32 *dst, const u32 *src, u32 len);
bool (*usrToKernelStrncpy)(char *dst, const char *src, u32 len);
bool (*kernelToUsr8)(void *dst, const void *src, u32 len);
bool (*kernelToUsr32)(u32 *dst, const u32 *src, u32 len);
bool (*kernelToUsrStrncpy)(char *dst, const char *src, u32 len);

void (*svcFallbackHandler)(u8 svcId);
u32 *officialSvcHandlerTail;

bool isN3DS;

u32 *exceptionStackTop;
void *kernelUsrCopyFuncsStart, *kernelUsrCopyFuncsEnd;

bool *isDevUnit;
bool *enableUserExceptionHandlersForCPUExc;

InterruptManager *interruptManager;
KBaseInterruptEvent *customInterruptEvent;

void (*flushEntireICache)(void);
void (*flushEntireDCacheAndL2C)(void); // reentrant, but unsafe in fatal exception contexts in case the kernel is f*cked up
void (*initFPU)(void);
void (*mcuReboot)(void);
void (*coreBarrier)(void);

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

CfwInfo cfwInfo;
