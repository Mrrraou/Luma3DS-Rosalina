#include "utils.h"
#include "fatalExceptionHandlers.h"
#include "memory.h"

bool isN3DS;
u8 *vramKMapping, *dspAndAxiWramMapping, *fcramKMapping, *exceptionRWMapping;
static u32 *exceptionsPage = (u32*)0xFFFF0000;

void (*initFPU)(void);
void (*mcuReboot)(void);

static void initMappingInfo(void)
{
    isN3DS = convertVAToPA((void*)0xE9000000) != 0; // check if there's the extra FCRAM
    if(convertVAToPA((void *)0xF0000000) != 0)
    {
        // Older mappings
        fcramKMapping = (u8*)0xF0000000;
        vramKMapping = (u8*)0xE8000000;
        dspAndAxiWramMapping = (u8*)0xEFF00000;
    }
    else
    {
        // Newer mappings
        fcramKMapping = (u8*)0xE0000000;
        vramKMapping = (u8*)0xD8000000;
        dspAndAxiWramMapping = (u8*)0xDFF00000;
    }

    exceptionRWMapping = dspAndAxiWramMapping + ((u32)convertVAToPA((void*)0xFFFF0000) - 0x1FF00000);
}

static void setupFatalExceptionHandlers(void)
{
    u32 *endPos = exceptionsPage + 0x400;

    u32 *initFPU_;
    for(initFPU_ = exceptionsPage; *initFPU_ != 0xE1A0D002 && initFPU_ < endPos; initFPU_++);
    initFPU_ += 3;

    u32 *freeSpace;
    for(freeSpace = initFPU_; *freeSpace != 0xFFFFFFFF && freeSpace < endPos; freeSpace++);
    freeSpace = (u32*)(dspAndAxiWramMapping + ((u32)convertVAToPA(freeSpace) - 0x1FF00000));

    u32 *mcuReboot_;
    for(mcuReboot_ = exceptionsPage; *mcuReboot_ != 0xE3A0A0C2 && mcuReboot_ < endPos; mcuReboot_++);
    mcuReboot_ -= 2;

    initFPU = (void (*)(void))initFPU_;
    mcuReboot = (void (*)(void))mcuReboot_;

    /*void (*flushDataCache)(const void *, u32) = (void*)0xFFF1D56C;
    void (*flushInstructionCache)(const void *, u32) = (void*)0xFFF1FCCC;
    flushDataCache(exceptionRWMapping, 0x1000);
    flushDataCache(exceptionsPage, 0x1000);*/

    flushCachesRange(exceptionRWMapping, 0x1000); // I'm being extra cautious here
    flushCachesRange(exceptionsPage, 0x1000);

    memcpy(freeSpace, fatalExceptionVeneers, 48);

    u32 *excRW = (u32 *)exceptionRWMapping;
    const u32 indices[] = {7, 1, 3, 4};
    for(u32 i = 0; i < 4; i++)
        excRW[indices[i]] = MAKE_BRANCH(excRW + indices[i], freeSpace + 3*i);

    flushDataCache(exceptionRWMapping, 0x1000);
    flushInstructionCache(exceptionsPage, 0x1000);
}

void main(void)
{
    initMappingInfo();
    setupFatalExceptionHandlers();
}
