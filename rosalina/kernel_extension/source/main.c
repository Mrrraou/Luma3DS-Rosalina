#include "utils.h"
#include "globals.h"
#include "synchronization.h"
#include "fatalExceptionHandlers.h"
#include "svc.h"
#include "svc/ConnectToPort.h"
#include "svcHandler.h"
#include "memory.h"

static const u32 *const exceptionsPage = (const u32 *)0xFFFF0000;
void *originalHandlers[7] = {NULL};

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

static void setupFatalExceptionHandlers(void)
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

    off = (u32 *)originalHandlers[(u32) SVC];
    while(*off++ != 0xE1A00009);
    svcFallbackHandler = (void (*)(u8))decodeARMBranch(off);
    for(;*off != 0xE8DD6F00; off++);
    officialSvcHandlerTail = off;
}

static void findUsefulSymbols(void)
{
    KProcessHandleTable__ToKProcess = (KProcess * (*)(KProcessHandleTable *, Handle))decodeARMBranch(5 + (u32 *)officialSVCs[0x76]);

    u32 *off = (u32 *)KProcessHandleTable__ToKProcess;
    while(*off != 0xE8BD80F0) off++;
    KProcessHandleTable__ToKAutoObject = (KAutoObject * (*)(KProcessHandleTable *, Handle))decodeARMBranch(off + 2);

    off = (u32 *)decodeARMBranch(3 + (u32 *)officialSVCs[9]); // KThread::Terminate
    while(*off != 0xE5C4007D) off++;
    KSynchronizationObject__Signal = (void (*)(KSynchronizationObject *, bool))decodeARMBranch(off + 3);

    off = (u32 *)officialSVCs[0x24];
    while(*off != 0xE59F004C) off++;
    WaitSynchronization1 = (Result (*)(void *, KThread *, KSynchronizationObject *, s64))decodeARMBranch(off + 6);

    off = (u32 *)officialSVCs[0x33];
    while(*off != 0xE20030FF) off++;
    KProcessHandleTable__CreateHandle = (Result (*)(KProcessHandleTable *, Handle *, KAutoObject *, u8))decodeARMBranch(off + 2);

    off = (u32 *)officialSVCs[0x7C];
    while(*off != 0x03530000) off++;
    KObjectMutex__WaitAndAcquire = (void (*)(KObjectMutex *))decodeARMBranch(++off);
    while(*off != 0xE320F000) off++;
    KObjectMutex__ErrorOccured = (void (*)(void))decodeARMBranch(off + 1);

    off = (u32 *)originalHandlers[(u32) DATA_ABORT];
    while(*off != (u32)exceptionStackTop) off++;
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
                    usrToKernel8 = (bool (*)(void *, const void *, u32))off;
                    break;
                case 2:
                    usrToKernel32 = (bool (*)(u32 *, const u32 *, u32))off;
                    break;
                case 3:
                    usrToKernelStrncpy = (bool (*)(char *, const char *, u32))off;
                    break;
                case 4:
                    kernelToUsr8 = (bool (*)(void *, const void *, u32))off;
                    break;
                case 5:
                    kernelToUsr32 = (bool (*)(u32 *, const u32 *, u32))off;
                    break;
                case 6:
                    kernelToUsrStrncpy = (bool (*)(char *, const char *, u32))off;
                    break;
                default: break;
            }
            n_cmp_0++;
        }
    }

    while(off[0] != 0xE3520000 || off[1] != 0x03A00000) off++;

    off = (u32 *)0xFFFF0000;
    while(*off != 0x96007F9) off++;
    isDevUnit = *(bool **)(off - 1);
    enableUserExceptionHandlersForCPUExc = *(bool **)(off + 1);
}

struct Parameters
{
    void (*SGI0HandlerCallback)(struct Parameters *, u32 *);
    InterruptManager *interruptManager;
    u32 *L2MMUTable; // bit31 mapping

    void (*flushEntireICache)(void);

    void (*flushEntireDCacheAndL2C)(void);
    void (*initFPU)(void);
    void (*mcuReboot)(void);
    void (*coreBarrier)(void);

    u32 *exceptionStackTop;
    u32 kernelVersion;

    CfwInfo cfwInfo;
};

u32 kernelVersion;

void enableDebugFeatures(void)
{
    *isDevUnit = true; // for debug SVCs and user exc. handlers, etc.

    u32 *off = (u32 *)officialSVCs[0x7C];
    while(off[0] != 0xE5D00001 || off[1] != 0xE3500000) off++;
    *(u32 *)PA_FROM_VA_PTR(off + 2) = 0xE1A00000; // in case 6: beq -> nop
}

void main(volatile struct Parameters *p)
{
    isN3DS = getNumberOfCores() == 4;
    interruptManager = p->interruptManager;

    flushEntireICache = p->flushEntireICache;
    flushEntireDCacheAndL2C = p->flushEntireDCacheAndL2C;
    initFPU = p->initFPU;
    mcuReboot = p->mcuReboot;
    coreBarrier = p->coreBarrier;

    exceptionStackTop = p->exceptionStackTop;
    kernelVersion = p->kernelVersion;
    cfwInfo = p->cfwInfo;

    setupSGI0Handler();
    setupFatalExceptionHandlers();
    findUsefulSymbols();
    enableDebugFeatures();

    *trampo_ = (u32)ConnectToPortHook;
}
