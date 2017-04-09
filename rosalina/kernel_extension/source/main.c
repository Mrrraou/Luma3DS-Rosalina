#include "utils.h"
#include "globals.h"
#include "synchronization.h"
#include "fatalExceptionHandlers.h"
#include "svc.h"
#include "svc/ConnectToPort.h"
#include "svcHandler.h"
#include "memory.h"

static const u32 *const exceptionsPage = (const u32 *)0xFFFF0000;
void *originalHandlers[8] = {NULL};

enum VECTORS { RESET = 0, UNDEFINED_INSTRUCTION, SVC, PREFETCH_ABORT, DATA_ABORT, RESERVED, IRQ, FIQ };

static void setupSGI0Handler(void)
{
    for(u32 i = 0; i < getNumberOfCores(); i++)
        interruptManager->N3DS.privateInterrupts[i][0].interruptEvent = customInterruptEvent;
}

static inline void **getHandlerDestination(enum VECTORS vector)
{
    u32 *branch_dst = (u32 *)decodeARMBranch((u32 *)exceptionsPage + (u32)vector);
    return (void **)(branch_dst + 2);
}

static inline void swapHandlerInVeneer(enum VECTORS vector, void *handler)
{
    void **dst = getHandlerDestination(vector);
    originalHandlers[(u32)vector] = *dst;
    if(handler != NULL)
        *(void**)PA_FROM_VA_PTR(dst) = handler;
}

static u32 *trampo_;

static void setupExceptionHandlers(void)
{
    swapHandlerInVeneer(FIQ, FIQHandler);
    swapHandlerInVeneer(UNDEFINED_INSTRUCTION, undefinedInstructionHandler);
    swapHandlerInVeneer(PREFETCH_ABORT, prefetchAbortHandler);
    swapHandlerInVeneer(DATA_ABORT, dataAbortHandler);

    swapHandlerInVeneer(SVC, svcHandler);

    void **arm11SvcTable = (void**)originalHandlers[(u32)SVC];
    while(*arm11SvcTable != NULL) arm11SvcTable++; //Look for SVC0 (NULL)
    memcpy(officialSVCs, arm11SvcTable, 4 * 0x7E);

    u32 *off;
    for(off = (u32 *)officialSVCs[0x2D]; *off != 0x65736162; off++);
    *(void **)PA_FROM_VA_PTR(arm11SvcTable + 0x2D) = officialSVCs[0x2D] = (void *)off[1];
    trampo_ = (u32 *)PA_FROM_VA_PTR(off + 3);

    CustomBackdoor = (Result (*)(void *, ...))*((u32 *)officialSVCs[0x2F] + 2);
    *(void **)PA_FROM_VA_PTR(arm11SvcTable + 0x2F) = officialSVCs[0x2F] = (void *)*((u32 *)officialSVCs[0x2F] + 2);

    off = (u32 *)originalHandlers[(u32) SVC];
    while(*off++ != 0xE1A00009);
    svcFallbackHandler = (void (*)(u8))decodeARMBranch(off);
    for(;*off != 0xE8DD6F00; off++);
    officialSvcHandlerTail = off;
}

static void findUsefulSymbols(void)
{
    KProcessHandleTable__ToKProcess = (KProcess * (*)(KProcessHandleTable *, Handle))decodeARMBranch(5 + (u32 *)officialSVCs[0x76]);

    u32 *off;

    for(off = (u32 *)KProcessHandleTable__ToKProcess; *off != 0xE8BD80F0; off++);
    KProcessHandleTable__ToKAutoObject = (KAutoObject * (*)(KProcessHandleTable *, Handle))decodeARMBranch(off + 2);

    for(off = (u32 *)decodeARMBranch(3 + (u32 *)officialSVCs[9]); /* KThread::Terminate */ *off != 0xE5C4007D; off++);
    KSynchronizationObject__Signal = (void (*)(KSynchronizationObject *, bool))decodeARMBranch(off + 3);

    for(off = (u32 *)officialSVCs[0x24]; *off != 0xE59F004C; off++);
    WaitSynchronization1 = (Result (*)(void *, KThread *, KSynchronizationObject *, s64))decodeARMBranch(off + 6);

    for(off = (u32 *)officialSVCs[0x33]; *off != 0xE20030FF; off++);
    KProcessHandleTable__CreateHandle = (Result (*)(KProcessHandleTable *, Handle *, KAutoObject *, u8))decodeARMBranch(off + 2);

    KProcessHandleTable__ToKThread = (KThread * (*)(KProcessHandleTable *, Handle))decodeARMBranch((u32 *)decodeARMBranch((u32 *)officialSVCs[0x37] + 3) /* GetThreadId */ + 5);

    for(off = (u32 *)officialSVCs[0x54]; *off != 0xE8BD8008; off++);
    flushDataCacheRange = (void (*)(void *, u32))(*((u32 *)off[1]) + 3);

    for(off = (u32 *)officialSVCs[0x71]; *off != 0xE2101102; off++);
    KProcessHwInfo__MapProcessMemory = (Result (*)(KProcessHwInfo *, KProcessHwInfo *, void *, void *, u32))decodeARMBranch(off - 1);

    for(off = (u32 *)officialSVCs[0x7C]; *off != 0x03530000; off++);
    KObjectMutex__WaitAndAcquire = (void (*)(KObjectMutex *))decodeARMBranch(++off);
    for(; *off != 0xE320F000; off++);
    KObjectMutex__ErrorOccured = (void (*)(void))decodeARMBranch(off + 1);

    for(off = (u32 *)originalHandlers[(u32) DATA_ABORT]; *off != (u32)exceptionStackTop; off++);
    kernelUsrCopyFuncsStart = (void *)off[1];
    kernelUsrCopyFuncsEnd = (void *)off[2];

    u32 n_cmp_0;
    for(off = (u32 *)kernelUsrCopyFuncsStart, n_cmp_0 = 1; n_cmp_0 <= 6; off++)
    {
        if(*off == 0xE3520000)
        {
            // We're missing some funcs
            switch(n_cmp_0)
            {
                case 1:
                    usrToKernelMemcpy8 = (bool (*)(void *, const void *, u32))off;
                    break;
                case 2:
                    usrToKernelMemcpy32 = (bool (*)(u32 *, const u32 *, u32))off;
                    break;
                case 3:
                    usrToKernelStrncpy = (s32 (*)(char *, const char *, u32))off;
                    break;
                case 4:
                    kernelToUsrMemcpy8 = (bool (*)(void *, const void *, u32))off;
                    break;
                case 5:
                    kernelToUsrMemcpy32 = (bool (*)(u32 *, const u32 *, u32))off;
                    break;
                case 6:
                    kernelToUsrStrncpy = (s32 (*)(char *, const char *, u32))off;
                    break;
                default: break;
            }
            n_cmp_0++;
        }
    }

    GetSystemInfo = (Result (*)(s64 *, s32, s32))decodeARMBranch((u32 *)officialSVCs[0x2A] + 3);
    GetProcessInfo = (Result (*)(s64 *, Handle, u32))decodeARMBranch((u32 *)officialSVCs[0x2B] + 3);
    GetThreadInfo = (Result (*)(s64 *, Handle, u32))decodeARMBranch((u32 *)officialSVCs[0x2C] + 3);
    ConnectToPort = (Result (*)(Handle *, const char*))decodeARMBranch((u32 *)officialSVCs[0x2D] + 3);
    OpenProcess = (Result (*)(Handle *, u32))decodeARMBranch((u32 *)officialSVCs[0x33] + 3);
    GetProcessId = (Result (*)(u32 *, Handle))decodeARMBranch((u32 *)officialSVCs[0x35] + 3);
    DebugActiveProcess = (Result (*)(Handle *, u32))decodeARMBranch((u32 *)officialSVCs[0x60] + 3);


    for(off = (u32 *)svcFallbackHandler; *off != 0xE8BD4010; off++);
    kernelpanic = (void (*)(void))off;

    for(off = (u32 *)0xFFFF0000; off[0] != 0xE3A01002 || off[1] != 0xE3A00004; off++);
    SignalDebugEvent = (Result (*)(DebugEventType type, u32 info, ...))decodeARMBranch(off + 2);

    for(; *off != 0x96007F9; off++);
    isDevUnit = *(bool **)(off - 1);
    enableUserExceptionHandlersForCPUExc = *(bool **)(off + 1);

    ///////////////////////////////////////////

    // Shitty/lazy heuristic but it works on even 4.5, so...
    u32 textStart = ((u32)originalHandlers[(u32) SVC]) & ~0xFFFF;
    u32 rodataStart = (u32)(interruptManager->N3DS.privateInterrupts[0][6].interruptEvent->vtable) & ~0xFFF;

    u32 textSize = rodataStart - textStart;
    for(off = (u32 *)textStart; off < (u32 *)(textStart + textSize) - 3; off++)
    {
        if(off[0] == 0xE3510B1A && off[1] == 0xE3A06000)
        {
            u32 *off2;
            for(off2 = off; *off2 != 0xE92D40F8; off2--);
            invalidateInstructionCacheRange = (void (*)(void *, u32))off2;
        }

        if(kernelVersion >= SYSTEM_VERSION(2, 53, 0) && off[0] == 0xE92D41F0 && off[1] == 0xE1A06000
            && off[2] == 0xE3A07000 && off[3] == 0xE2860020)
            KTimerAndWDTManager__Sanitize = (void (*)(KTimerAndWDTManager *))off;

    }

    if(kernelVersion < SYSTEM_VERSION(2, 53, 0)) KTimerAndWDTManager__Sanitize = NULL;
}

struct Parameters
{
    void (*SGI0HandlerCallback)(struct Parameters *, u32 *);
    InterruptManager *interruptManager;
    u32 *L2MMUTable; // bit31 mapping

    void (*initFPU)(void);
    void (*mcuReboot)(void);
    void (*coreBarrier)(void);

    u32 *exceptionStackTop;
    u32 kernelVersion;

    CfwInfo cfwInfo;
};

u32 kernelVersion;

static void enableDebugFeatures(void)
{
    // Also patch kernelpanic with bkpt 0xFFFE
    *isDevUnit = true; // for debug SVCs and user exc. handlers, etc.

    u32 *off = (u32 *)officialSVCs[0x7C];
    while(off[0] != 0xE5D00001 || off[1] != 0xE3500000) off++;
    *(u32 *)PA_FROM_VA_PTR(off + 2) = 0xE1A00000; // in case 6: beq -> nop

    off = (u32 *)DebugActiveProcess;
    while(*off != 0xE3110001) off++;
    *(u32 *)PA_FROM_VA_PTR(off) = 0xE3B01001; // tst r1, #1 -> movs r1, #1
}

static void doOtherPatches(void)
{
    u32 *kpanic = (u32 *)kernelpanic;
    u32 *sanitize = (u32 *)(u32 *)KTimerAndWDTManager__Sanitize;
    *(u32 *)PA_FROM_VA_PTR(kpanic) = 0xE12FFF7E; // bkpt 0xFFFE
    if(sanitize != NULL)
        *(u32 *)PA_FROM_VA_PTR(sanitize) = 0xE12FFF1E; // bx lr
}

void main(volatile struct Parameters *p)
{
    isN3DS = getNumberOfCores() == 4;
    interruptManager = p->interruptManager;

    initFPU = p->initFPU;
    mcuReboot = p->mcuReboot;
    coreBarrier = p->coreBarrier;

    exceptionStackTop = p->exceptionStackTop;
    kernelVersion = p->kernelVersion;
    cfwInfo = p->cfwInfo;

    setupSGI0Handler();
    setupExceptionHandlers();
    findUsefulSymbols();
    enableDebugFeatures();
    doOtherPatches();

    *trampo_ = (u32)ConnectToPortHookWrapper;
}
