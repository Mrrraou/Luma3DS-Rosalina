#include "kernel_extension.h"
#include "kernel.h"
#include "utils.h"

#define MPCORE_REGS_BASE        ((u32)PA_PTR(0x17E00000))
#define MPCORE_GID_REGS_BASE    (MPCORE_REGS_BASE + 0x1000)
#define MPCORE_GID_SGI          (*(vu32 *)(MPCORE_GID_REGS_BASE + 0xF00))

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


static void K_SGI0HandlerCallback(volatile struct Parameters *p)
{
    vu32 *L1MMUTable;

    __asm__ volatile("cpsid aif"); // disable interrupts

    p->coreBarrier();
    p->flushEntireDCacheAndL2C();

    __asm__ volatile("mrc p15, 0, %0, c2, c0, 1" : "=r"(L1MMUTable));
    L1MMUTable = (vu32 *)(((u32)L1MMUTable & ~0x3FFF) | (1 << 31));

    // Actually map the kernel ext
    u32 L2MMUTableAddr = (u32)(p->L2MMUTable) & ~(1 << 31);
    L1MMUTable[0x40000000 >> 20] = L2MMUTableAddr | 1;

    p->flushEntireICache(); // this does a DSB too
    ((void (*)(volatile struct Parameters *))0x40000000)(p);

    p->flushEntireDCacheAndL2C();
    p->flushEntireICache();
}

u32 ALIGN(0x400) L2MMUTableFor0x40000000[256] = {0};
static void K_ConfigureAndSendSGI0ToAllCores(void)
{
    // see /patches/k11MainHook.s
    u32 *off;
    u32 *flushEntireDCacheAndL2C, *initFPU, *mcuReboot, *coreBarrier;

    // Search for stuff in the 0xFFFF0000 page
    for(initFPU = (u32 *)0xFFFF0000; initFPU < (u32 *)0xFFFF1000 && *initFPU != 0xE1A0D002; initFPU++);
    initFPU += 3;

    for(mcuReboot = initFPU; mcuReboot < (u32 *)0xFFFF1000 && *mcuReboot != 0xE3A0A0C2; mcuReboot++);
    mcuReboot--;
    flushEntireDCacheAndL2C = (u32 *)decodeARMBranch(mcuReboot - 5);
    coreBarrier = (u32 *)decodeARMBranch(mcuReboot - 4);

    for(off = mcuReboot; off < (u32 *)0xFFFF1000 && *off != 0x726C6468; off++); // "hdlr"

    volatile struct Parameters *p = (struct Parameters *)PA_FROM_VA_PTR(off); // Caches? What are caches?
    p->SGI0HandlerCallback = (void (*)(struct Parameters *, u32 *))PA_FROM_VA_PTR(K_SGI0HandlerCallback);
    p->L2MMUTable = (u32 *)PA_FROM_VA_PTR(L2MMUTableFor0x40000000);
    p->flushEntireICache = (void (*) (void))PA_FROM_VA_PTR(flushEntireICache);
    p->flushEntireDCacheAndL2C = (void (*) (void))flushEntireDCacheAndL2C;
    p->initFPU = (void (*) (void))initFPU;
    p->mcuReboot = (void (*) (void))mcuReboot;
    p->coreBarrier = (void (*) (void))coreBarrier;

    // Now let's configure the L2 table

    //4KB extended small pages: [SYS:RW USR:-- X  TYP:NORMAL SHARED OUTER NOCACHE, INNER CACHED WB WA]
    u32 kernel_extension_pa = (u32)convertVAToPA(kernel_extension);
    for(u32 offset = 0; offset < kernel_extension_size; offset += 0x1000)
        L2MMUTableFor0x40000000[offset >> 12] = (kernel_extension_pa + offset) | 0x516;

    MPCORE_GID_SGI = 0xF0000; // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/CACGDJJC.html
}

void installKernelExtension(void)
{
    svc_7b(K_ConfigureAndSendSGI0ToAllCores);
}
