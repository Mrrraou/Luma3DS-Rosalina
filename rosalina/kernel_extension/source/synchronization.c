#include "synchronization.h"
#include "utils.h"
#include "kernel.h"

u32 getNumberOfCores(void)
{
    return (MPCORE_SCU_CFG & 3) + 1;
}

SGI0Callback_t SGI0HandlersPerCore[4] = {NULL};
u32 SGI0ParametersPerCore[4] = {0};

KInterruptEvent *SGI0Handler(InterruptEvent UNUSED *this, u32 UNUSED interruptID)
{
    u32 sourceCore = (MPCORE_INT_ACK >> 10) & 3;

    SGI0Callback_t cb = SGI0HandlersPerCore[sourceCore];
    cb(sourceCore, SGI0ParametersPerCore[sourceCore]);

    return NULL;
}

void executeFunctionOnCores(SGI0Callback_t func, u8 targetList, u8 targetListFilter, u32 parameter)
{
    u32 coreID = getCurrentCoreID();
    SGI0HandlersPerCore[coreID] = func;
    SGI0ParametersPerCore[coreID] = parameter;

    enableIRQ(); // make sure interrupts are enabled
    MPCORE_GID_SGI = (targetListFilter << 24) | (targetList << 16) | 0;
}
