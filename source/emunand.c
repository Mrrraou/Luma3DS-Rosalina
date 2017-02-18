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
*   Code for locating the SDMMC struct by Normmatt
*/

#include "emunand.h"
#include "memory.h"
#include "fatfs/sdmmc/sdmmc.h"
#include "../build/bundled.h"

u32 emuOffset;

void locateEmuNand(u32 *emuHeader, FirmwareSource *nandType)
{
    static u8 __attribute__((aligned(4))) temp[0x200];
    static u32 nandSize = 0,
               fatStart;

    if(!nandSize)
    {
        nandSize = getMMCDevice(0)->total_size;
        sdmmc_sdcard_readsectors(0, 1, temp);
        fatStart = *(u32 *)(temp + 0x1C6); //First sector of the FAT partition
    }

    for(u32 i = 0; i < 3; i++)
    {
        static const u32 roundedMinsizes[] = {0x1D8000, 0x26E000};

        u32 nandOffset;
        switch(i)
        {
            case 1:
                nandOffset = ROUND_TO_4MB(nandSize + 1); //"Default" layout
                break;
            case 2:
                nandOffset = roundedMinsizes[ISN3DS ? 1 : 0]; //"Minsize" layout
                break;
            case 0:
                nandOffset = *nandType == FIRMWARE_EMUNAND ? 0 : (nandSize > 0x200000 ? 0x400000 : 0x200000); //"Legacy" layout
                break;
        }

        if(*nandType != FIRMWARE_EMUNAND) nandOffset *= ((u32)*nandType - 1);

        if(fatStart >= nandOffset + roundedMinsizes[ISN3DS ? 1 : 0])
        {
            //Check for RedNAND
            if(!sdmmc_sdcard_readsectors(nandOffset + 1, 1, temp) && memcmp(temp + 0x100, "NCSD", 4) == 0)
            {
                emuOffset = nandOffset + 1;
                *emuHeader = nandOffset + 1;
                return;
            }

            //Check for Gateway EmuNAND
            else if(i != 2 && !sdmmc_sdcard_readsectors(nandOffset + nandSize, 1, temp) && memcmp(temp + 0x100, "NCSD", 4) == 0)
            {
                emuOffset = nandOffset;
                *emuHeader = nandOffset + nandSize;
                return;
            }
        }

        if(*nandType == FIRMWARE_EMUNAND) break;
    }

    //Fallback to the first EmuNAND if there's no second/third/fourth one, or to SysNAND if there isn't any
    if(*nandType != FIRMWARE_EMUNAND)
    {
        *nandType = FIRMWARE_EMUNAND;
        locateEmuNand(emuHeader, nandType);
    }
    else *nandType = FIRMWARE_SYSNAND;
}

static inline bool getFreeK9Space(u8 *pos, u32 size, u8 **freeK9Space)
{
    const u8 pattern[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

    //Looking for the last free space before Process9
    *freeK9Space = memsearch(pos, pattern, size, sizeof(pattern));

    if(*freeK9Space == NULL) return false;

    *freeK9Space += 0x455;

    return true;
}

static inline u32 getSdmmc(u8 *pos, u32 size, u32 *sdmmc)
{
    //Look for struct code
    const u8 pattern[] = {0x21, 0x20, 0x18, 0x20};

    const u8 *off = memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    *sdmmc = *(u32 *)(off + 9) + *(u32 *)(off + 0xD);

    return 0;
}

static inline u32 patchNandRw(u8 *pos, u32 size, u32 branchOffset)
{
    //Look for read/write code
    const u8 pattern[] = {0x1E, 0x00, 0xC8, 0x05};

    u16 *readOffset = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(readOffset == NULL) return 1;

    readOffset -= 3;

    u16 *writeOffset = (u16 *)memsearch((u8 *)(readOffset + 5), pattern, 0x100, sizeof(pattern));

    if(writeOffset == NULL) return 1;

    writeOffset -= 3;
    *readOffset = *writeOffset = 0x4C00;
    readOffset[1] = writeOffset[1] = 0x47A0;
    ((u32 *)writeOffset)[1] = ((u32 *)readOffset)[1] = branchOffset;

    return 0;
}

static inline u32 patchMpu(u8 *pos, u32 size)
{
    //Look for MPU pattern
    const u8 pattern[] = {0x03, 0x00, 0x24, 0x00};

    u16 *off = (u16 *)memsearch(pos, pattern, size, sizeof(pattern));

    if(off == NULL) return 1;

    off[1] = 0x0036;
    off[0xC] = off[0x12] = 0x0603;

    return 0;
}

u32 patchEmuNand(u8 *arm9Section, u32 kernel9Size, u8 *process9Offset, u32 process9Size, u32 emuHeader, u8 *kernel9Address)
{
    u8 *freeK9Space;

    if(!getFreeK9Space(arm9Section, kernel9Size, &freeK9Space)) return 1;

    u32 ret = 0;

    //Copy EmuNAND code
    memcpy(freeK9Space, emunand_bin, emunand_bin_size);

    //Add the data of the found EmuNAND
    u32 *posOffset = (u32 *)memsearch(freeK9Space, "NAND", emunand_bin_size, 4),
        *posHeader = (u32 *)memsearch(freeK9Space, "NCSD", emunand_bin_size, 4);
    *posOffset = emuOffset;
    *posHeader = emuHeader;

    //Find and add the SDMMC struct
    u32 *posSdmmc = (u32 *)memsearch(freeK9Space, "SDMC", emunand_bin_size, 4);
    u32 sdmmc;
    ret += getSdmmc(process9Offset, process9Size, &sdmmc);
    if(!ret) *posSdmmc = sdmmc;

    //Add EmuNAND hooks
    u32 branchOffset = (u32)(freeK9Space - arm9Section + kernel9Address);
    ret += patchNandRw(process9Offset, process9Size, branchOffset);

    //Set MPU
    ret += patchMpu(arm9Section, kernel9Size);

    return ret;
}