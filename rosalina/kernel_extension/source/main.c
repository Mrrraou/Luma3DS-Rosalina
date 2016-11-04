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

enum VECTORS { RESET = 0, UNDEFINED_INSTRUCTION, SVC, PREFETCH_ABORT, DATA_ABORT, RESERVED, IRQ, FIQ };

static inline u32 swapLdrLiteralInHandler(enum VECTORS vector, u32 value)
{
    s32 branch_dst_offset = ((u32*)0xFFFF0000)[vector] & 0xFFFFFF;
    if(branch_dst_offset & 0x800000)
        branch_dst_offset |= ~0xFFFFFF;
    branch_dst_offset += 2; // pc is always 2 instructions ahead
    u32 *branch_dst = (u32*)(0xFFFF0000 + (sizeof(u32) * vector) + (branch_dst_offset * sizeof(u32)));
    branch_dst = (u32*)(dspAndAxiWramMapping + ((u32)convertVAToPA(branch_dst) - 0x1FF00000));
    u32 ret = branch_dst[2];
    branch_dst[2] = value;
    flushCachesRange(branch_dst, 0x3); // lol
    return ret;
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

    /*memcpy(freeSpace, fatalExceptionVeneers, 48);

    u32 *excRW = (u32 *)exceptionRWMapping;
    const u32 indices[] = {7, 1, 3, 4};
    for(u32 i = 0; i < 4; i++)
        excRW[indices[i]] = MAKE_BRANCH(excRW + indices[i], freeSpace + 3*i);*/

    swapLdrLiteralInHandler(FIQ, (u32)FIQHandler);
    swapLdrLiteralInHandler(UNDEFINED_INSTRUCTION, (u32)undefinedInstructionHandler);
    //swapLdrLiteralInHandler(PREFETCH_ABORT, (u32)prefetchAbortHandler);
    swapLdrLiteralInHandler(DATA_ABORT, (u32)dataAbortHandler);

    /*flushDataCache(exceptionRWMapping, 0x1000);
    flushInstructionCache(exceptionsPage, 0x1000);*/
    flushCachesRange(exceptionRWMapping, 0x1000); // I'm being extra cautious here
    flushCachesRange(exceptionsPage, 0x1000);
}

void main(void)
{
    initMappingInfo();
    setupFatalExceptionHandlers();
}
