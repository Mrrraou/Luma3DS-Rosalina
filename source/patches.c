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

#include "patches.h"
#include "memory.h"
#include "config.h"
#include "../build/rebootpatch.h"
#include "../build/svcGetCFWInfopatch.h"
#include "../build/twl_k11modulespatch.h"
#include "../build/mmuHookpatch.h"
#include "../build/k11MainHookpatch.h"
#include "../build/svcBackdoorspatch.h"
#include "utils.h"

#define MAKE_BRANCH_LINK(src,dst) (0xEB000000 | ((u32)((((u8 *)(dst) - (u8 *)(src)) >> 2) - 2) & 0xFFFFFF))

u8 *getProcess9(u8 *pos, u32 size, u32 *process9Size, u32 *process9MemAddr)
{
    u8 *off = memsearch(pos, "ess9", size, 4);

    *process9Size = *(u32 *)(off - 0x60) * 0x200;
    *process9MemAddr = *(u32 *)(off + 0xC);

    //Process9 code offset (start of NCCH + ExeFS offset + ExeFS header size)
    return off - 0x204 + (*(u32 *)(off - 0x64) * 0x200) + 0x200;
}

u32 *getKernel11Info(u8 *pos, u32 size, u32 *baseK11VA, u8 **freeK11Space, u32 **arm11SvcHandler, u32 **arm11ExceptionsPage)
{
    const u8 pattern[] = {0x00, 0xB0, 0x9C, 0xE5};

    *arm11ExceptionsPage = (u32 *)memsearch(pos, pattern, size, sizeof(pattern));

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

    memcpy(*freeK11Space, mmuHook, mmuHook_size);
    *off = MAKE_BRANCH_LINK(off, *freeK11Space);

    (*freeK11Space) += mmuHook_size;
}

void installK11MainHook(u8 *pos, u32 size, u32 baseK11VA, u32 *arm11SvcTable, u32 *arm11ExceptionsPage, u8 **freeK11Space)
{
    const u8 pattern[] = {0x00, 0x00, 0xA0, 0xE1, 0x03, 0xF0, 0x20, 0xE3, 0xFD, 0xFF, 0xFF, 0xEA};

    u32 *off = (u32 *)memsearch(pos, pattern, size, 12);
    // look for cpsie i and place our function call in the nop 2 instructions before
    while(*off != 0xF1080080) off--;
    off -= 2;

    memcpy(*freeK11Space, k11MainHook, k11MainHook_size);

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

    off = (u32 *)memsearch(*freeK11Space, "bind", k11MainHook_size, 4);

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
    //if(ISA9LH) info->flags = 1 << 1;
    /* if(ISN3DS) info->flags |= 1 << 4;
    if(isSafeMode) info->flags |= 1 << 5;*/
    //TODO: update codebase to use those flags ^; add isK9LH flag

    (*freeK11Space) += k11MainHook_size;
}
void patchSignatureChecks(u8 *pos, u32 size)
{
    const u16 sigPatch[2] = {0x2000, 0x4770};

    //Look for signature checks
    const u8 pattern[] = {0xC0, 0x1C, 0x76, 0xE7},
             pattern2[] = {0xB5, 0x22, 0x4D, 0x0C};

    u16 *off = (u16 *)memsearch(pos, pattern, size, 4),
        *off2 = (u16 *)(memsearch(pos, pattern2, size, 4) - 1);

    *off = sigPatch[0];
    off2[0] = sigPatch[0];
    off2[1] = sigPatch[1];
}

void patchFirmlaunches(u8 *pos, u32 size, u32 process9MemAddr)
{
    //Look for firmlaunch code
    const u8 pattern[] = {0xE2, 0x20, 0x20, 0x90};

    u8 *off = memsearch(pos, pattern, size, 4) - 0x13;

    //Firmlaunch function offset - offset in BLX opcode (A4-16 - ARM DDI 0100E) + 1
    u32 fOpenOffset = (u32)(off + 9 - (-((*(u32 *)off & 0x00FFFFFF) << 2) & (0xFFFFFF << 2)) - pos + process9MemAddr);

    //Copy firmlaunch code
    memcpy(off, reboot, reboot_size);

    //Put the fOpen offset in the right location
    u32 *pos_fopen = (u32 *)memsearch(off, "OPEN", reboot_size, 4);
    *pos_fopen = fOpenOffset;
}

void patchFirmWrites(u8 *pos, u32 size)
{
    const u16 writeBlock[2] = {0x2000, 0x46C0};

    //Look for FIRM writing code
    u8 *const off1 = memsearch(pos, "exe:", size, 4);
    const u8 pattern[] = {0x00, 0x28, 0x01, 0xDA};

    u16 *off2 = (u16 *)memsearch(off1 - 0x100, pattern, 0x100, 4);

    off2[0] = writeBlock[0];
    off2[1] = writeBlock[1];
}

void patchOldFirmWrites(u8 *pos, u32 size)
{
    const u16 writeBlockOld[2] = {0x2400, 0xE01D};

    //Look for FIRM writing code
    const u8 pattern[] = {0x04, 0x1E, 0x1D, 0xDB};

    u16 *off = (u16 *)memsearch(pos, pattern, size, 4);

    off[0] = writeBlockOld[0];
    off[1] = writeBlockOld[1];
}

u8 patchK11ModuleLoading(u32 section0size, u32 moduleSize, u8 *startPos, u32 size)
{
  const u8 moduleAmountPattern[]  = {0x05, 0x00, 0x57, 0xE3}; // cmp r7, 5
  const u8 moduleAmountPatch[]    = {0x06, 0x00, 0x57, 0xE3}; // cmp r7, 6

  const u8 modulePidPattern[] =  {0x00, 0xF0, 0x20, 0xE3,  // nop
                                  0x05, 0x00, 0xA0, 0xE3}; // mov r0, #5
  const u8 modulePidPatch[]   =  {0x00, 0xF0, 0x20, 0xE3,  // nop
                                  0x06, 0x00, 0xA0, 0xE3}; // mov r0, #6

  // This needs to be patched, or K11 won't like it.
  u32 maxModuleDst      = 0xDFF00000 + section0size - 4;
  u32 maxModuleDstPatch = maxModuleDst + moduleSize;
  u32 maxModuleSectionSize      = section0size;
  u32 maxModuleSectionSizePatch = section0size + moduleSize;

  u8 *off;
  if(!(off = memsearch(startPos, moduleAmountPattern, size, 4)))
    return 1;
  memcpy(off, moduleAmountPatch, 4);

  if(!(off = memsearch(startPos, modulePidPattern, size, 8)))
    return 2;
  memcpy(off, modulePidPatch, 8);

  if(!(off = memsearch(startPos, (u8*)&maxModuleDst, size, 4)))
    return 3;
  memcpy(off, (u8*)&maxModuleDstPatch, 4);

  if(!(off = memsearch(startPos, (u8*)&maxModuleSectionSize, size, 4)))
    return 4;
  memcpy(off, (u8*)&maxModuleSectionSizePatch, 4);

  return 0;
}

void reimplementSvcBackdoorAndImplementCustomBackdoor(u32 *arm11SvcTable, u8 **freeK11Space, u32 *arm11ExceptionsPage)
{
    if(!arm11SvcTable[0x7B])
    {
        memcpy(*freeK11Space, svcBackdoors, 40);

        arm11SvcTable[0x7B] = 0xFFFF0000 + *freeK11Space - (u8 *)arm11ExceptionsPage;
        (*freeK11Space) += 40;
    }

    memcpy(*freeK11Space, svcBackdoors + 40, svcBackdoors_size - 40);
    arm11SvcTable[0x2F] = 0xFFFF0000 + *freeK11Space - (u8 *)arm11ExceptionsPage;
    (*freeK11Space) += (svcBackdoors_size - 40);
}

void patchTitleInstallMinVersionCheck(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x0A, 0x81, 0x42, 0x02};

    u8 *off = memsearch(pos, pattern, size, 4);

    if(off != NULL) off[4] = 0xE0;
}

void applyLegacyFirmPatches(u8 *pos, FirmwareType firmType)
{
    const patchData twlPatches[] = {
        {{0x1650C0, 0x165D64}, {{ 6, 0x00, 0x20, 0x4E, 0xB0, 0x70, 0xBD }}, 0},
        {{0x173A0E, 0x17474A}, { .type1 = 0x2001 }, 1},
        {{0x174802, 0x17553E}, { .type1 = 0x2000 }, 2},
        {{0x174964, 0x1756A0}, { .type1 = 0x2000 }, 2},
        {{0x174D52, 0x175A8E}, { .type1 = 0x2001 }, 2},
        {{0x174D5E, 0x175A9A}, { .type1 = 0x2001 }, 2},
        {{0x174D6A, 0x175AA6}, { .type1 = 0x2001 }, 2},
        {{0x174E56, 0x175B92}, { .type1 = 0x2001 }, 1},
        {{0x174E58, 0x175B94}, { .type1 = 0x4770 }, 1}
    },
    agbPatches[] = {
        {{0x9D2A8, 0x9DF64}, {{ 6, 0x00, 0x20, 0x4E, 0xB0, 0x70, 0xBD }}, 0},
        {{0xD7A12, 0xD8B8A}, { .type1 = 0xEF26 }, 1}
    };

    /* Calculate the amount of patches to apply. Only count the boot screen patch for AGB_FIRM
       if the matching option was enabled (keep it as last) */
    u32 numPatches = firmType == TWL_FIRM ? (sizeof(twlPatches) / sizeof(patchData)) :
                                            (sizeof(agbPatches) / sizeof(patchData) - !CONFIG(6));
    const patchData *patches = firmType == TWL_FIRM ? twlPatches : agbPatches;

    //Patch
    for(u32 i = 0; i < numPatches; i++)
    {
        switch(patches[i].type)
        {
            case 0:
                memcpy(pos + patches[i].offset[isN3DS ? 1 : 0], patches[i].patch.type0 + 1, patches[i].patch.type0[0]);
                break;
            case 2:
                *(u16 *)(pos + patches[i].offset[isN3DS ? 1 : 0] + 2) = 0;
            case 1:
                *(u16 *)(pos + patches[i].offset[isN3DS ? 1 : 0]) = patches[i].patch.type1;
                break;
        }
    }
}

void patchTwlBg(u8 *pos)
{
    u8 *dst = pos + (isN3DS ? 0xFEA4 : 0xFCA0);

    memcpy(dst, twl_k11modules, twl_k11modules_size); //Install K11 hook

    u32 *off = (u32 *)memsearch(dst, "LAUN", twl_k11modules_size, 4);
    *off = isN3DS ? 0xCDE88 : 0xCD5F8; //Dev SRL launcher offset

    u16 *src1 = (u16 *)(pos + (isN3DS ? 0xE38 : 0xE3C)),
        *src2 = (u16 *)(pos + (isN3DS ? 0xE54 : 0xE58));

    //Construct BLX instructions:
    src1[0] = 0xF000 | ((((u32)dst - (u32)src1 - 4) & (0xFFF << 11)) >> 12);
    src1[1] = 0xE800 | ((((u32)dst - (u32)src1 - 4) & 0xFFF) >> 1);

    src2[0] = 0xF000 | ((((u32)dst - (u32)src2 - 4) & (0xFFF << 11)) >> 12);
    src2[1] = 0xE800 | ((((u32)dst - (u32)src2 - 4) & 0xFFF) >> 1);
}

void patchArm9ExceptionHandlersInstall(u8 *pos, u32 size)
{
    const u8 pattern[] = {
        0x18, 0x10, 0x80, 0xE5,
        0x10, 0x10, 0x80, 0xE5,
        0x20, 0x10, 0x80, 0xE5,
        0x28, 0x10, 0x80, 0xE5,
    }; //i.e when it stores ldr pc, [pc, #-4]

    u32* off = (u32 *)(memsearch(pos, pattern, size, sizeof(pattern)));
    if(off == NULL) return;
    off += sizeof(pattern)/4;

    for(u32 r0 = 0x08000000; *off != 0xE3A01040; off++) //Until mov r1, #0x40
    {
        if((*off >> 26) != 0x39 || ((*off >> 16) & 0xF) != 0 || ((*off >> 25) & 1) != 0 || ((*off >> 20) & 5) != 0)
            continue; //Discard everything that's not str rX, [r0, #imm](!)

        int rD = (*off >> 12) & 0xF,
            offset = (*off & 0xFFF) * ((((*off >> 23) & 1) == 0) ? -1 : 1),
            writeback = (*off >> 21) & 1,
            pre = (*off >> 24) & 1;

        u32 addr = r0 + ((pre || !writeback) ? offset : 0);
        if((addr & 7) != 0 && addr != 0x08000014 && addr != 0x08000004)
            *off = 0xE1A00000; //nop
        else
            *off = 0xE5800000 | (rD << 12) | (addr & 0xFFF); //Preserve IRQ and SVC handlers

        if(!pre) addr += offset;
        if(writeback) r0 = addr;
    }
}

void patchSvcBreak9(u8 *pos, u32 size, u32 k9Address)
{
    //Stub svcBreak with "bkpt 65535" so we can debug the panic.
    //Thanks @yellows8 and others for mentioning this idea on #3dsdev.
    const u8 svcHandlerPattern[] = {0x00, 0xE0, 0x4F, 0xE1}; //mrs lr, spsr

    u32 *arm9SvcTable = (u32 *)memsearch(pos, svcHandlerPattern, size, 4);
    while(*arm9SvcTable) arm9SvcTable++; //Look for SVC0 (NULL)

    u32 *addr = (u32 *)(pos + arm9SvcTable[0x3C] - k9Address);
    *addr = 0xE12FFF7F;
}

void patchKernel9Panic(u8 *pos, u32 size, FirmwareType firmType)
{
    if(firmType == TWL_FIRM || firmType == AGB_FIRM)
    {
        u8 *off = pos + (isN3DS ? 0x723C : 0x69A8);
        *(u16 *)off = 0x4778;           //bx pc
        *(u16 *)(off + 2) = 0x46C0;     //nop
        *(u32 *)(off + 4) = 0xE12FFF7E; //bkpt 65534
    }
    else
    {
        const u8 pattern[] = {0x00, 0x20, 0xA0, 0xE3, 0x02, 0x30, 0xA0, 0xE1, 0x02, 0x10, 0xA0, 0xE1, 0x05, 0x00, 0xA0, 0xE3};

        u32 *off = (u32 *)memsearch(pos, pattern, size, 16);
        *off = 0xE12FFF7E;
    }
}

void patchKernel11Panic(u8 *pos, u32 size)
{
    const u8 pattern[] = {0x02, 0x0B, 0x44, 0xE2, 0x00, 0x10, 0x90, 0xE5};

    u32 *off = (u32 *)memsearch(pos, pattern, size, 8);
    *off = 0xE12FFF7E;
}

void patchArm11SvcAccessChecks(u32 *arm11SvcHandler)
{
    while(*arm11SvcHandler != 0xE11A0E1B) arm11SvcHandler++; //TST R10, R11,LSL LR
    *arm11SvcHandler = 0xE3B0A001; //MOVS R10, #1
}

void patchN3DSK11ProcessorAffinityChecks(u8 *pos, u32 size)
{
    const u8 pattern[] =   {0x0F, 0x2C, 0x01, 0xE2, 0x03, 0x0C, 0x52, 0xE3, 0x03, 0x00, 0x00, 0x0A, 0x02};

    u8 *off = memsearch(pos, pattern, size, 13);
    off[11] = 0xEA; //BEQ -> B

    off = memsearch(pos + 16, pattern, size, 13);
    off[11] = 0xEA; //BEQ -> B
}

void patchP9AccessChecks(u8 *pos, u32 size)
{
    const u8 pattern[] = {0xE0, 0x00, 0x40, 0x39, 0x08, 0x58};

    u16 *off = (u16 *)memsearch(pos, pattern, size, 6) - 7;

    off[0] = 0x2001; //mov r0, #1
    off[1] = 0x4770; //bx lr
}

void patchUnitInfoValueSet(u8 *pos, u32 size)
{
    //Look for UNITINFO value being set during kernel sync
    const u8 pattern[] = {0x01, 0x10, 0xA0, 0x13};

    u8 *off = memsearch(pos, pattern, size, 4);

    off[0] = isDevUnit ? 0 : 1;
    off[3] = 0xE3;
}
