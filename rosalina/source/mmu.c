#include "mmu.h"
#include "utils.h"

static inline void mapL2TranslationTable(u32 *ttb, u32 L1Addr, u32 *L2Table)
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

extern u8 *dspAndAxiWramMapping;
void K_mapRosalinaKernelExtension(void)
{
    if(*(u32*)((u32)convertVAToPA(L2MMUTableFor0x40000000) | (1 << 31)) != L2MMUTableFor0x40000000[0]) __asm__ volatile("bkpt 1");
    u32 *ttb1VA = (u32 *)((u32)getTTB1Address() - 0x1FF00000 + (u32)dspAndAxiWramMapping);
    constructL2TranslationTable(L2MMUTableFor0x40000000, 0, kernel_extension, kernel_extension_size, 0x516);
    // ^ 4KB extended small page: [SYS:RW USR:-- X  TYP:NORMAL SHARED OUTER NOCACHE, INNER CACHED WB WA]

    // 0xA0000000 = VA to which part of Rosalina will be remapped (TTBCR >= 2 so anything >= 0x40000000 is fine)
    mapL2TranslationTable(ttb1VA, 0x40000000, L2MMUTableFor0x40000000);

    finishMMUReconfiguration();
}

void mapRosalinaKernelExtension(void)
{
    svc_7b(K_mapRosalinaKernelExtension);
}
