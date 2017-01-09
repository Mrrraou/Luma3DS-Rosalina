#include "utils.h"
#include "synchronization.h"
#include "fatalExceptionHandlers.h"
#include "svc.h"
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

    u32 *off = (u32 *)originalHandlers[(u32) SVC];
    while(*off++ != 0xE1A00009);
    svcFallbackHandler = (void (*)(u8))decodeARMBranch(off);
    for(;*off != 0xE8DD6F00; off++);
    officialSvcHandlerTail = off;
}

static void findUsefulFunctions(void)
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
    WaitSynchronization1 = (Result (*)(void *, KThread *, KSynchronizationObject *, s64))(off + 6);
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

     CfwInfo cfwInfo;
};

void main(volatile struct Parameters *p)
{
    isN3DS = getNumberOfCores() == 4;
    interruptManager = p->interruptManager;

    flushEntireICache = p->flushEntireICache;
    flushEntireDCacheAndL2C = p->flushEntireDCacheAndL2C;
    initFPU = p->initFPU;
    mcuReboot = p->mcuReboot;
    coreBarrier = p->coreBarrier;

    cfwInfo = p->cfwInfo;

    setupSGI0Handler();
    setupFatalExceptionHandlers();
    findUsefulFunctions();
}
