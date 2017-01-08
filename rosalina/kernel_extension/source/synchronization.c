#include "synchronization.h"
#include "utils.h"
#include "kernel.h"

u32 getNumberOfCores(void)
{
    return (MPCORE_SCU_CFG & 3) + 1;
}

extern SGI0Handler_t SGI0Handler;

void executeFunctionOnCores(SGI0Handler_t handler, u8 targetList, u8 targetListFilter)
{
    u32 coreID = getCurrentCoreID();
    SGI0Handler = handler;

    if(targetListFilter == 0 && (targetListFilter & (1 << coreID)) != 0)
        enableIRQ(); // make sure interrupts are enabled
    MPCORE_GID_SGI = (targetListFilter << 24) | (targetList << 16) | 0;
}
