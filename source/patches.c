/*
*   This file is part of Luma3DS
*   Copyright (C) 2016 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b of GPLv3 applies to this file: Requiring preservation of specified
*   reasonable legal notices or author attributions in that material or in the Appropriate Legal
*   Notices displayed by works containing it.
*/

/*
*   Signature patches by an unknown author
*   firmlaunches patching code originally by delebile
*   FIRM partition writes patches by delebile
*   ARM11 modules patching code originally by Subv
*   Idea for svcBreak patches from yellows8 and others on #3dsdev
*/

#include "patches.h"
#include "fs.h"
#include "exceptions.h"
#include "memory.h"
#include "config.h"
#include "utils.h"
#include "../build/bundled.h"

static inline void pathChanger(u8 *pos)
{
    const char *pathFile = "path.txt";
    u8 path[57];

    u32 pathSize = fileRead(path, pathFile, sizeof(path));

    if(pathSize < 6) return;

    if(path[pathSize - 1] == 0xA) pathSize--;
    if(path[pathSize - 1] == 0xD) pathSize--;

    if(pathSize < 6 || pathSize > 55 || path[0] != '/' || memcmp(path + pathSize - 4, ".bin", 4) != 0) return;

    u16 finalPath[56];
    for(u32 i = 0; i < pathSize; i++)
        finalPath[i] = (u16)path[i];

    finalPath[pathSize] = 0;

    u8 *posPath = memsearch(pos, u"sd", reboot_bin_size, 4) + 0xA;
    memcpy(posPath, finalPath, (pathSize + 1) * 2);
}

u8 *getProcess9Info(u8 *pos, u32 size, u32 *process9Size, u32 *process9MemAddr)
{
    u8 *temp = memsearch(pos, "NCCH", size, 4);

    if(temp == NULL) error("Failed to get Process9 data.");

    Cxi *off = (Cxi *)(temp - 0x100);

    *process9Size = (off->ncch.exeFsSize - 1) * 0x200;
    *process9MemAddr = off->exHeader.systemControlInfo.textCodeSet.address;

    return (u8 *)off + (off->ncch.exeFsOffset + 1) * 0x200;
}

u32 *getKernel11Info(u8 *pos, u32 size, u32 *baseK11VA, u8 **freeK11Space, u32 **arm11SvcHandler, u32 **arm11ExceptionsPage)
{
    const u8 pattern[] = {0x00, 0xB0, 0x9C, 0xE5};
    *arm11ExceptionsPage = (u32 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(*arm11ExceptionsPage == NULL) error("Failed to get Kernel11 data.");

    u32 *arm11SvcTable;

    *arm11ExceptionsPage -= 0xB;
    u32 svcOffset = (-(((*arm11ExceptionsPage)[2] & 0xFFFFFF) << 2) & (0xFFFFFF << 2)) - 8; //Branch offset + 8 for prefetch
    u32 pointedInstructionVA = 0xFFFF0008 - svcOffset;
    *baseK11VA = pointedInstructionVA & 0xFFFF0000; //This assumes that the pointed instruction has an offset < 0x10000, iirc that's always the case
    arm11SvcTable = *arm11SvcHandler = (u32 *)(pos + *(u32 *)(pos + pointedInstructionVA - *baseK11VA + 8) - *baseK11VA); //SVC handler address
    while(*arm11SvcTable) arm11SvcTable++; //Look for SVC0 (NULL)

    u32 *freeSpace;
    for(freeSpace = *arm11ExceptionsPage; freeSpace < *arm11ExceptionsPage + 0x400 && *freeSpace != 0xFFFFFFFF; freeSpace++);
    *freeK11Space = (u8 *) freeSpace;

    return arm11SvcTable;
}

void installMMUHook(u8 *pos, u32 size, u8 **freeK11Space)
{
    const u8 pattern[] = {0x0E, 0x32, 0xA0, 0xE3, 0x02, 0xC2, 0xA0, 0xE3};

    u32 *off = (u32 *)memsearch(pos, pattern, size, 8);

    memcpy(*freeK11Space, mmuHook_bin, mmuHook_bin_size);
    *off = MAKE_BRANCH_LINK(off, *freeK11Space);

    (*freeK11Space) += mmuHook_bin_size;
}

void installK11MainHook(u8 *pos, u32 size, bool isSafeMode, u32 baseK11VA, u32 *arm11SvcTable, u32 *arm11ExceptionsPage, u8 **freeK11Space)
{
    const u8 pattern[] = {0x00, 0x00, 0xA0, 0xE1, 0x03, 0xF0, 0x20, 0xE3, 0xFD, 0xFF, 0xFF, 0xEA};

    u32 *off = (u32 *)memsearch(pos, pattern, size, 12);
    // look for cpsie i and place our function call in the nop 2 instructions before
    while(*off != 0xF1080080) off--;
    off -= 2;

    memcpy(*freeK11Space, k11MainHook_bin, k11MainHook_bin_size);

    u32 relocBase = 0xFFFF0000 + (*freeK11Space - (u8 *)arm11ExceptionsPage);
    *off = MAKE_BRANCH_LINK(baseK11VA + ((u8 *)off - pos), relocBase);

    off = (u32 *)(pos +  (arm11SvcTable[0x50] - baseK11VA)); //svcBindInterrupt
    while(off[0] != 0xE1A05000 || off[1] != 0xE2100102 || off[2] != 0x5A00000B) off++;
    off--;

    signed int offset = (*off & 0xFFFFFF) << 2;
    offset = offset << 6 >> 6; // sign extend
    offset += 8;

    u32 InterruptManager_mapInterrupt = baseK11VA + ((u8 *)off - pos) + offset;
    u32 interruptManager = *(u32 *)(off - 4 + (*(off - 6) & 0xFFF) / 4);

    off = (u32 *)memsearch(*freeK11Space, "bind", k11MainHook_bin_size, 4);

    *off++ = InterruptManager_mapInterrupt;

    // Relocate stuff
    *off++ += relocBase;
    *off++ += relocBase;
    off++;
    *off++ = interruptManager;

    off += 6;

    struct CfwInfo
    {
        char magic[4];

        u8 versionMajor;
        u8 versionMinor;
        u8 versionBuild;
        u8 flags;

        u32 commitHash;

        u32 config;
    } __attribute__((packed)) *info = (struct CfwInfo *)off;

    const char *rev = REVISION;
    memcpy(&info->magic, "LUMA", 4);
    info->commitHash = COMMIT_HASH;
    info->config = configData.config;
    info->versionMajor = (u8)(rev[1] - '0');
    info->versionMinor = (u8)(rev[3] - '0');

    bool isRelease;

    if(rev[4] == '.')
    {
        info->versionBuild = (u8)(rev[5] - '0');
        isRelease = rev[6] == 0;
    }
    else isRelease = rev[4] == 0;

    if(isRelease) info->flags = 1;
    if(ISN3DS) info->flags |= 1 << 4;
    if(isSafeMode) info->flags |= 1 << 5;
    if(ISA9LH) info->flags |= 1 << 6;

    (*freeK11Space) += k11MainHook_bin_size;
}

void installSvcConnectToPortInitHook(u32 *arm11SvcTable, u32 *arm11ExceptionsPage, u8 **freeK11Space)
{
    u32 addr = 0xFFFF0000 + (u32)*freeK11Space - (u32)arm11ExceptionsPage;
    u32 svcSleepThreadAddr = arm11SvcTable[0x0A], svcConnectToPortAddr = arm11SvcTable[0x2D];

    arm11SvcTable[0x2D] = addr;
    memcpy(*freeK11Space, svcConnectToPortInitHook_bin, svcConnectToPortInitHook_bin_size);

    u32 *off = (u32 *)memsearch(*freeK11Space, "orig", svcConnectToPortInitHook_bin_size, 4);
    off[0] = svcConnectToPortAddr;
    off[1] = svcSleepThreadAddr;

    (*freeK11Space) += svcConnectToPortInitHook_bin_size;
}


void installSvcCustomBackdoor(u32 *arm11SvcTable, u8 **freeK11Space, u32 *arm11ExceptionsPage)
{
    memcpy(*freeK11Space, svcCustomBackdoor_bin, svcCustomBackdoor_bin_size);
    *((u32 *)*freeK11Space + 1) = arm11SvcTable[0x2F]; // temporary location
    arm11SvcTable[0x2F] = 0xFFFF0000 + *freeK11Space - (u8 *)arm11ExceptionsPage;
    (*freeK11Space) += svcCustomBackdoor_bin_size;
}

u32 patchSignatureChecks(u8 *pos, u32 size)
{
    //Look for signature checks
    const u8 pattern[] = {0xC0, 0x1C, 0x76, 0xE7},
             pattern2[] = {0xB5, 0x22, 0x4D, 0x0C};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));
    u8 *temp = memsearch(pos, pattern2, size, sizeof(pattern2));

    if(off == NULL || temp == NULL) return 1;

    u16 *off2 = (u16 *)(temp - 1);
    *off = off2[0] = 0x2000;
    off2[1] = 0x4770;

    return 0;
}

u32 patchOldSignatureChecks(u8 *pos, u32 size)
{
    // Look for signature checks
    const u8 pattern[] = {0xC0, 0x1C, 0xBD, 0xE7},
             pattern2[] = {0xB5, 0x23, 0x4E, 0x0C};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));
    u8 *temp = memsearch(pos, pattern2, size, sizeof(pattern2));

    if(off == NULL || temp == NULL) return 1;

    u16 *off2 = (u16 *)(temp - 1);
    *off = off2[0] = 0x2000;
    off2[1] = 0x4770;

    return 0;
}

u32 patchFirmlaunches(u8 *pos, u32 size, u32 process9MemAddr)
{
    //Look for firmlaunch code
    const u8 pattern[] = {0xE2, 0x20, 0x20, 0x90};

    u8 *off = memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off -= 0x13;

    //Firmlaunch function offset - offset in BLX opcode (A4-16 - ARM DDI 0100E) + 1
    u32 fOpenOffset = (u32)(off + 9 - (-((*(u32 *)off & 0x00FFFFFF) << 2) & (0xFFFFFF << 2)) - pos + process9MemAddr);

    //Copy firmlaunch code
    memcpy(off, reboot_bin, reboot_bin_size);

    //Put the fOpen offset in the right location
    u32 *pos_fopen = (u32 *)memsearch(off, "OPEN", reboot_bin_size, 4);
    *pos_fopen = fOpenOffset;

    if(CONFIG(USECUSTOMPATH)) pathChanger(off);

    return 0;
}

u32 patchFirmWrites(u8 *pos, u32 size)
{
    //Look for FIRM writing code
    u8 *off = memsearch(pos, "exe:", size, 4);

    if(off == NULL) return 1;

    const u8 pattern[] = {0x00, 0x28, 0x01, 0xDA};

    u16 *off2 = (u16 *)memsearch(off - 0x100, pattern, 0x100, sizeof(pattern));

    if(off2 == NULL) return 1;

    off2[0] = 0x2000;
    off2[1] = 0x46C0;

    return 0;
}

u32 patchOldFirmWrites(u8 *pos, u32 size)
{
    //Look for FIRM writing code
    const u8 pattern[] = {0x04, 0x1E, 0x1D, 0xDB};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[0] = 0x2400;
    off[1] = 0xE01D;

    return 0;
}

u32 patchTitleInstallMinVersionChecks(u8 *pos, u32 size, u32 firmVersion)
{
    const u8 pattern[] = {0xFF, 0x00, 0x00, 0x02};

    u8 *off = memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return firmVersion == 0xFFFFFFFF ? 0 : 1;

    off++;

    //Zero out the first TitleID in the list
    memset32(off, 0, 8);

    return 0;
}

u32 patchZeroKeyNcchEncryptionCheck(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x28, 0x2A, 0xD0, 0x08};

    u8 *temp = memsearch(pos, pattern, size, sizeof(pattern));

    if(temp == NULL) return 1;

    u16 *off = (u16 *)(temp - 1);
    *off = 0x2001; //mov r0, #1

    return 0;
}

u32 patchNandNcchEncryptionCheck(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x07, 0xD1, 0x28, 0x7A};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off--;
    *off = 0x2001; //mov r0, #1

    return 0;
}

u32 patchCheckForDevCommonKey(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x03, 0x7C, 0x28, 0x00};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    *off = 0x2301; //mov r3, #1

    return 0;
}

u32 patchK11ModuleLoading(u32 section0size, u32 moduleSize, u8 *startPos, u32 size)
{
    const u8 moduleAmountPattern[]  = {0x05, 0x00, 0x57, 0xE3}; // cmp r7, #5

    const u8 modulePidPattern[] = {
        0x00, 0xF0, 0x20, 0xE3, // nop
        0x05, 0x00, 0xA0, 0xE3, // mov r0, #5
    };

    // This needs to be patched, or Kernel11 won't like it.
    u32 maxModuleDst = 0xDFF00000 + section0size - 4;
    u32 maxModuleDstPatch = maxModuleDst + moduleSize;
    u32 maxModuleSectionSize = section0size;
    u32 maxModuleSectionSizePatch = section0size + moduleSize;

    u8 *off = memsearch(startPos, moduleAmountPattern, size, 4);
    if(!off) return 1;
    *off = 6;

    off = memsearch(startPos, modulePidPattern, size, 8);
    if(!off) return 1;
    *(off + 4) = 6;

    off = memsearch(startPos, &maxModuleDst, size, 4);
    if(!off) return 1;
    *(u32 *)off = maxModuleDstPatch;

    off = memsearch(startPos, &maxModuleSectionSize, size, 4);
    if(!off) return 1;
    *(u32 *)off = maxModuleSectionSizePatch;

    return 0;
}

void patchMPUTable(u8 *pos, u32 size, u8 *arm9SectionDst)
{
    const u8 mpuTablePattern[] = {
        0x10, 0xCF, 0x12, 0xEE, // mrc p15, 0, r12, c2, c0, 0
        0x30, 0x1F, 0x12, 0xEE, // mrc p15, 0, r1, c2, c0, 1
        0x10, 0x0F, 0x13, 0xEE, // mrc p15, 0, r2, c3, c0, 0
        0x00, 0x20, 0xA0, 0xE3, // mov r2, #0
    };

    void *off = memsearch(pos, mpuTablePattern, size, sizeof(mpuTablePattern));
    off += sizeof(mpuTablePattern);

    u32 *ldr = (u32*)off;

    u32 ldrLiteralOffset = (*ldr & 0xFFF);
    u8 **ldrLiteral = (u8**)((u8*)(ldr + 2) + ldrLiteralOffset);

    u8 *mpuTable = *ldrLiteral - arm9SectionDst + pos;

    for(u32 i = 0; i < 8; i++)
    {
        mpuTable[3 * sizeof(u32) * i + 4] = 3; // Write data access: rw / rw
        mpuTable[3 * sizeof(u32) * i + 5] = 3; // Write instruction access: rw / rw
    }
}

void injectPxiHook(u8 *pos, u32 size)
{
    const u8 pxiDevDefaultCasePattern[] = {
        0x84, 0xD0, 0x8D, 0xE2, // add sp, sp, #0x84
        0xF0, 0x8F, 0xBD, 0xE8, // ldmfd sp!, {r4-r11, pc}
        0x40, 0x00, 0xA0, 0xE3, // mov r0, #0x40
        0x00, 0x00, 0x84, 0xE5, // str r0, [r4]
    };
	const u8 pxiDevDefaultCasePatch[] = {
        0x04, 0xE0, 0x8F, 0xE2, // add lr, pc, #4
        0x04, 0xF0, 0x1F, 0xE5, // ldr pc, [pc, -#4]
    };

    // The arm9 section starts at 0x08006000 on New3DS... available space is
    // fairly limited.
	fileRead((void*)0x08001000, "/luma/starbit.bin", 0x5000);

	void *off = memsearch(pos, pxiDevDefaultCasePattern, size, sizeof(pxiDevDefaultCasePattern));
    off += 2 * sizeof(u32);

	memcpy(off, pxiDevDefaultCasePatch, sizeof(pxiDevDefaultCasePatch));
	*(u32*)(off + sizeof(pxiDevDefaultCasePatch)) = 0x08001000;
}


u32 patchArm9ExceptionHandlersInstall(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x80, 0xE5, 0x40, 0x1C};

    u8 *temp = memsearch(pos, pattern, size, sizeof(pattern));

    if(temp == NULL) return 1;

    u32 *off = (u32 *)(temp - 0xA);

    for(u32 r0 = 0x08000000; *off != 0xE3A01040; off++) //Until mov r1, #0x40
    {
        //Discard everything that's not str rX, [r0, #imm](!)
        if((*off & 0xFE5F0000) != 0xE4000000) continue;

        u32 rD = (*off >> 12) & 0xF,
            offset = (*off & 0xFFF) * ((((*off >> 23) & 1) == 0) ? -1 : 1);
        bool writeback = ((*off >> 21) & 1) != 0,
             pre = ((*off >> 24) & 1) != 0;

        u32 addr = r0 + ((pre || !writeback) ? offset : 0);
        if((addr & 7) != 0 && addr != 0x08000014 && addr != 0x08000004) *off = 0xE1A00000; //nop
        else *off = 0xE5800000 | (rD << 12) | (addr & 0xFFF); //Preserve IRQ and SVC handlers

        if(!pre) addr += offset;
        if(writeback) r0 = addr;
    }

    return 0;
}

u32 patchSvcBreak9(u8 *pos, u32 size, u32 kernel9Address)
{
    //Stub svcBreak with "bkpt 65535" so we can debug the panic

    //Look for the svc handler
    const u8 pattern[] = {0x00, 0xE0, 0x4F, 0xE1}; //mrs lr, spsr

    u32 *arm9SvcTable = (u32 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(arm9SvcTable == NULL) return 1;

    while(*arm9SvcTable != 0) arm9SvcTable++; //Look for SVC0 (NULL)

    u32 *addr = (u32 *)(pos + arm9SvcTable[0x3C] - kernel9Address);
    *addr = 0xE12FFF7F;

    return 0;
}

u32 patchKernel9Panic(u8 *pos, u32 size)
{
    const u8 pattern[] = {0xFF, 0xEA, 0x04, 0xD0};

    u8 *temp = memsearch(pos, pattern, size, sizeof(pattern));

    if(temp == NULL) return 1;

    u32 *off = (u32 *)(temp - 0x12);
    *off = 0xE12FFF7E;

    return 0;
}

u32 patchP9AccessChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x00, 0x08, 0x49, 0x68};

    u8 *temp = memsearch(pos, pattern, size, sizeof(pattern));

    if(temp == NULL) return 1;

    u16 *off = (u16 *)(temp - 3);
    off[0] = 0x2001; //mov r0, #1
    off[1] = 0x4770; //bx lr

    return 0;
}

u32 patchUnitInfoValueSet(u8 *pos, u32 size)
{
    //Look for UNITINFO value being set during kernel sync
    const u8 pattern[] = {0x01, 0x10, 0xA0, 0x13};

    u8 *off = memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[0] = ISDEVUNIT ? 0 : 1;
    off[3] = 0xE3;

    return 0;
}

u32 patchLgySignatureChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x47, 0xC1, 0x17, 0x49};

    u8 *temp = memsearch(pos, pattern, size, sizeof(pattern));

    if(temp == NULL) return 1;

    u16 *off = (u16 *)(temp + 1);
    off[0] = 0x2000;
    off[1] = 0xB04E;
    off[2] = 0xBD70;

    return 0;
}

u32 patchTwlInvalidSignatureChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x20, 0xF6, 0xE7, 0x7F};

    u8 *temp = memsearch(pos, pattern, size, sizeof(pattern));

    if(temp == NULL) return 1;

    u16 *off = (u16 *)(temp - 1);
    *off = 0x2001; //mov r0, #1

    return 0;
}

u32 patchTwlNintendoLogoChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0xC0, 0x30, 0x06, 0xF0};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[1] = 0x2000;
    off[2] = 0;

    return 0;
}

u32 patchTwlWhitelistChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x22, 0x00, 0x20, 0x30};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[2] = 0x2000;
    off[3] = 0;

    return 0;
}

u32 patchTwlFlashcartChecks(u8 *pos, u32 size, u32 firmVersion)
{
    const u8 pattern[] = {0x25, 0x20, 0x00, 0x0E};

    u8 *temp = memsearch(pos, pattern, size, sizeof(pattern));

    if(temp == NULL)
    {
        if(firmVersion == 0xFFFFFFFF) return patchOldTwlFlashcartChecks(pos, size);

        return 1;
    }

    u16 *off = (u16 *)(temp + 3);
    off[0] = off[6] = off[0xC] = 0x2001; //mov r0, #1
    off[1] = off[7] = off[0xD] = 0; //nop

    return 0;
}

u32 patchOldTwlFlashcartChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x06, 0xF0, 0xA0, 0xFD};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[0] = off[6] = 0x2001; //mov r0, #1
    off[1] = off[7] = 0; //nop

    return 0;
}

u32 patchTwlShaHashChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x10, 0xB5, 0x14, 0x22};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[0] = 0x2001; //mov r0, #1
    off[1] = 0x4770;

    return 0;
}

u32 patchAgbBootSplash(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x00, 0x00, 0x01, 0xEF};

    u8 *off = memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[2] = 0x26;

    return 0;
}
