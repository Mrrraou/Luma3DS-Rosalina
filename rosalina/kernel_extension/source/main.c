#include "utils.h"
#include "synchronization.h"
#include "fatalExceptionHandlers.h"
#include "svc.h"
#include "memory.h"

bool isN3DS;
static const u32 *const exceptionsPage = (const u32 *)0xFFFF0000;
void *originalHandlers[7] = {NULL};
void *officialSVCs[0x7E] = {NULL};

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

static void overrideSVCList(void **arm11SvcTable)
{
    memcpy(officialSVCs, arm11SvcTable, 4 * 0x7E);
    for(u32 i = 0; i < overrideListSize; i++)
        *(void**)PA_FROM_VA_PTR(arm11SvcTable + overrideList[i].index) = overrideList[i].func;
}

static void setupFatalExceptionHandlers(void)
{
    swapHandlerInVeneer(FIQ, FIQHandler);
    swapHandlerInVeneer(UNDEFINED_INSTRUCTION, undefinedInstructionHandler);
    swapHandlerInVeneer(PREFETCH_ABORT, prefetchAbortHandler);
    swapHandlerInVeneer(DATA_ABORT, dataAbortHandler);

    swapHandlerInVeneer(SVC, NULL); //NULL so it's not replaced

    void **arm11SvcTable = (void**)originalHandlers[(u32)SVC];
    while(*arm11SvcTable != NULL) arm11SvcTable++; //Look for SVC0 (NULL)
    overrideSVCList(arm11SvcTable);
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

    setupSGI0Handler();
    setupFatalExceptionHandlers();
}
