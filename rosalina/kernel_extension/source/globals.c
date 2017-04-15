#include "globals.h"

KProcess * (*KProcessHandleTable__ToKProcess)(KProcessHandleTable *this, Handle processHandle);
KThread * (*KProcessHandleTable__ToKThread)(KProcessHandleTable *this, Handle threadHandle);
KAutoObject * (*KProcessHandleTable__ToKAutoObject)(KProcessHandleTable *this, Handle handle);
void (*KSynchronizationObject__Signal)(KSynchronizationObject *this, bool isPulse);
Result (*WaitSynchronization1)(void *this_unused, KThread *thread, KSynchronizationObject *syncObject, s64 timeout);
Result (*KProcessHandleTable__CreateHandle)(KProcessHandleTable *this, Handle *out, KAutoObject *obj, u8 token);
Result (*KProcessHwInfo__MapProcessMemory)(KProcessHwInfo *this, KProcessHwInfo *other, void *dst, void *src, u32 nbPages);
void (*KObjectMutex__WaitAndAcquire)(KObjectMutex *this);
void (*KObjectMutex__ErrorOccured)(void);

Result (*ControlMemory)(u32 *addrOut, u32 addr0, u32 addr1, u32 size, MemOp op, MemPerm perm, bool isLoader);
Result (*GetSystemInfo)(s64 *out, s32 type, s32 param);
Result (*GetProcessInfo)(s64 *out, Handle processHandle, u32 type);
Result (*GetThreadInfo)(s64 *out, Handle threadHandle, u32 type);
Result (*ConnectToPort)(Handle *out, const char *name);
Result (*OpenProcess)(Handle *out, u32 processId);
Result (*GetProcessId)(u32 *out, Handle process);
Result (*DebugActiveProcess)(Handle *out, u32 processId);

void (*flushDataCacheRange)(void *addr, u32 len);
void (*invalidateInstructionCacheRange)(void *addr, u32 len);

bool (*usrToKernelMemcpy8)(void *dst, const void *src, u32 len);
bool (*usrToKernelMemcpy32)(u32 *dst, const u32 *src, u32 len);
s32 (*usrToKernelStrncpy)(char *dst, const char *src, u32 len);
bool (*kernelToUsrMemcpy8)(void *dst, const void *src, u32 len);
bool (*kernelToUsrMemcpy32)(u32 *dst, const u32 *src, u32 len);
s32 (*kernelToUsrStrncpy)(char *dst, const char *src, u32 len);

Result (*CustomBackdoor)(void *function, ...);

void (*svcFallbackHandler)(u8 svcId);
void (*kernelpanic)(void);
void (*PostprocessSvc)(void);

Result (*SignalDebugEvent)(DebugEventType type, u32 info, ...);

bool isN3DS;

u32 *exceptionStackTop;
void *kernelUsrCopyFuncsStart, *kernelUsrCopyFuncsEnd;

bool *isDevUnit;
bool *enableUserExceptionHandlersForCPUExc;

InterruptManager *interruptManager;
KBaseInterruptEvent *customInterruptEvent;

void (*initFPU)(void);
void (*mcuReboot)(void);
void (*coreBarrier)(void);

CfwInfo cfwInfo;
