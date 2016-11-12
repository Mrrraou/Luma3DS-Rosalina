#include "mmu.h"
#include "utils.h"

static inline void mapL2TranslationTable(volatile u32 *ttb, u32 L1Addr, u32 *L2Table)
{
    ttb[L1Addr >> 20] = ((u32)convertVAToPA(L2Table) & ~0x3FF) | 1;
}

static void constructL2TranslationTable(u32 *table, u32 offset, const void *src, u32 size, u32 attributes)
{
    // Note: 4KB extended small pages are assumed

    u32 pa = (u32) convertVAToPA(src) & ~0xFFF;

    size = ((size + 0xFFF) >> 12) << 12;
    for(u32 i = 0; i < (size >> 12); i++)
        table[(offset >> 12) + i] = (pa + (i << 12)) | attributes;
}

u32 ALIGN(0x400) L2MMUTableFor0x40000000[256] = { 0 }; // page faults by default

static void K_constructL2TranslationTableForRosalina(void)
{
    constructL2TranslationTable(L2MMUTableFor0x40000000, 0, kernel_extension, kernel_extension_size, 0x516);
    // ^ 4KB extended small pages: [SYS:RW USR:-- X  TYP:NORMAL SHARED OUTER NOCACHE, INNER CACHED WB WA]
}

void constructL2TranslationTableForRosalina(void)
{
    svc_7b(K_constructL2TranslationTableForRosalina);
    assertSuccess(svcFlushProcessDataCache((Handle)0xFFFF8001, L2MMUTableFor0x40000000, sizeof(L2MMUTableFor0x40000000)));
    assertSuccess(svcFlushProcessDataCache((Handle)0xFFFF8001, kernel_extension, kernel_extension_size));
}

static void K_mapAndInstallRosalinaKernelExtension(void)
{
    vu32 *ttb1 = (vu32 *) PA_PTR(getTTB1Address());

    // 0x40000000 = VA to which part of Rosalina will be remapped (TTBCR >= 2 so anything >= 0x40000000 is fine)
    mapL2TranslationTable(ttb1, 0x40000000, L2MMUTableFor0x40000000); //only 1 table because Rosalina will always be less than 1MB
    dsb(); // needed, see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360e/CACBFJFA.html

    ((void (*)(void))0x40000000)();
}

void mapAndInstallRosalinaKernelExtension(void)
{
    svc_7b(K_mapAndInstallRosalinaKernelExtension);
}
