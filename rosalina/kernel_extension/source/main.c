#include "utils.h"
#include "fatalExceptionHandlers.h"
#include "svc/hooks.h"
#include "memory.h"

bool isN3DS;
u8 *vramKMapping, *dspAndAxiWramMapping, *fcramKMapping, *exceptionRWMapping;
static u32 *exceptionsPage = (u32*)0xFFFF0000;
u32 *arm11SvcTable;

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

static inline void* getHandlerDestination(enum VECTORS vector)
{
    s32 branch_dst_offset = exceptionsPage[vector] & 0xFFFFFF;
    if(branch_dst_offset & 0x800000)
        branch_dst_offset |= ~0xFFFFFF;
    branch_dst_offset += 2; // pc is always 2 instructions ahead
    u32 *branch_dst = exceptionsPage + vector + branch_dst_offset;
    branch_dst = (u32*)(dspAndAxiWramMapping + ((u32)convertVAToPA(branch_dst) - 0x1FF00000));
    return (void*)branch_dst[2];
}

static inline u32 swapLdrLiteralInHandler(enum VECTORS vector, u32 value)
{
    s32 branch_dst_offset = exceptionsPage[vector] & 0xFFFFFF;
    if(branch_dst_offset & 0x800000)
        branch_dst_offset |= ~0xFFFFFF;
    branch_dst_offset += 2; // pc is always 2 instructions ahead
    u32 *branch_dst = exceptionsPage + vector + branch_dst_offset;
    branch_dst = (u32*)(dspAndAxiWramMapping + ((u32)convertVAToPA(branch_dst) - 0x1FF00000));
    u32 ret = branch_dst[2];
    branch_dst[2] = value;
    flushCachesRange(branch_dst, 0x3); // lol
    return ret;
}

static void setupSvcHooks(void)
{
    u32 *svcTableRW = (u32*)(dspAndAxiWramMapping + ((u32)convertVAToPA(arm11SvcTable) - 0x1FF00000));
    #define INSTALL_HOOK(orig, hook, syscall) \
        (orig) = (void*)svcTableRW[(syscall)]; \
        svcTableRW[(syscall)] = (u32)(hook);

    INSTALL_HOOK(svcSleepThread, svcSleepThreadHook, 0xA);
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
    void (*flushInstructionCache)(const void *, u32) = (void*)0xFFF1FCCC;*/

    flushCachesRange(exceptionRWMapping, 0x1000); // I'm being extra cautious here
    flushCachesRange(exceptionsPage, 0x1000);

    swapLdrLiteralInHandler(FIQ, (u32)FIQHandler);
    //swapLdrLiteralInHandler(UNDEFINED_INSTRUCTION, (u32)undefinedInstructionHandler);
    swapLdrLiteralInHandler(PREFETCH_ABORT, (u32)prefetchAbortHandler);
    swapLdrLiteralInHandler(DATA_ABORT, (u32)dataAbortHandler);
    void *svcHandler = getHandlerDestination(SVC);
    arm11SvcTable = (u32*)svcHandler;
    while(*arm11SvcTable) arm11SvcTable++; //Look for SVC0 (NULL)
    setupSvcHooks();

    flushCachesRange(exceptionRWMapping, 0x1000); // I'm being extra cautious here
    flushCachesRange(exceptionsPage, 0x1000);
}

void main(void)
{
    initMappingInfo();
    setupFatalExceptionHandlers();
}
