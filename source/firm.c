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

#include "firm.h"
#include "config.h"
#include "utils.h"
#include "fs.h"
#include "patches.h"
#include "memory.h"
#include "cache.h"
#include "emunand.h"
#include "crypto.h"
#include "exceptions.h"
#include "draw.h"
#include "screen.h"
#include "buttons.h"
#include "pin.h"
#include "../build/injector.h"

extern u16 launchedFirmTidLow[8]; //Defined in start.s

static firmHeader *const firm = (firmHeader *)0x24000000;
static const firmSectionHeader *section;

u32 emuOffset,
    rosalinaModuleSize;

u8* topLeftFbPtr;

bool isN3DS,
     isDevUnit,
     isFirmlaunch;

cfgData configData;
FirmwareSource firmSource;

void main(void)
{
    bool isA9lh;

    u32 configTemp,
        emuHeader;

    FirmwareType firmType;
    FirmwareSource nandType;
    ConfigurationStatus needConfig;

    //Detect the console being used
    isN3DS = PDN_MPCORE_CFG == 7;

    //Detect dev units
    isDevUnit = CFG_UNITINFO != 0;

    //Mount filesystems. CTRNAND will be mounted only if/when needed
    mountFs();

    const char configPath[] = "/luma/config.bin";

    //Attempt to read the configuration file
    needConfig = readConfig(configPath) ? MODIFY_CONFIGURATION : CREATE_CONFIGURATION;

    if(DEV_OPTIONS != 2)
    {
        detectAndProcessExceptionDumps();
        installArm9Handlers();
    }

    //Determine if this is a firmlaunch boot
    if(launchedFirmTidLow[5] != 0)
    {
        if(needConfig == CREATE_CONFIGURATION) mcuReboot();

        isFirmlaunch = true;

        //'0' = NATIVE_FIRM, '1' = TWL_FIRM, '2' = AGB_FIRM
        firmType = launchedFirmTidLow[7] == u'3' ? SAFE_FIRM : (FirmwareType)(launchedFirmTidLow[5] - u'0');

        nandType = (FirmwareSource)BOOTCONFIG(0, 3);
        firmSource = (FirmwareSource)BOOTCONFIG(2, 1);
        isA9lh = BOOTCONFIG(3, 1) != 0;
    }
    else
    {
        //Get pressed buttons
        u32 pressed = HID_PAD;

        isFirmlaunch = false;
        firmType = NATIVE_FIRM;

        //Determine if booting with A9LH
        isA9lh = !PDN_SPI_CNT;

        //Save old options and begin saving the new boot configuration
        configTemp = (configData.config & 0xFFFFFFC0) | ((u32)isA9lh << 3);

        //If it's a MCU reboot, try to force boot options
        if(isA9lh && CFG_BOOTENV)
        {
            //Always force a sysNAND boot when quitting AGB_FIRM
            if(CFG_BOOTENV == 7)
            {
                nandType = FIRMWARE_SYSNAND;
                firmSource = CONFIG(1) ? FIRMWARE_SYSNAND : (FirmwareSource)BOOTCONFIG(2, 1);
                needConfig = DONT_CONFIGURE;

                //Flag to prevent multiple boot options-forcing
                configTemp |= 1 << 4;
            }

            /* Else, force the last used boot options unless a button is pressed
               or the no-forcing flag is set */
            else if(needConfig != CREATE_CONFIGURATION && !pressed && !BOOTCONFIG(4, 1))
            {
                nandType = (FirmwareSource)BOOTCONFIG(0, 3);
                firmSource = (FirmwareSource)BOOTCONFIG(2, 1);
                needConfig = DONT_CONFIGURE;
            }
        }

        //Boot options aren't being forced
        if(needConfig != DONT_CONFIGURE)
        {
            bool pinExists = CONFIG(8) && verifyPin();

            //If no configuration file exists or SELECT is held, load configuration menu
            bool shouldLoadConfigMenu = needConfig == CREATE_CONFIGURATION || ((pressed & BUTTON_SELECT) && !(pressed & BUTTON_L1));

            if(shouldLoadConfigMenu)
            {
                configMenu(pinExists);

                topLeftFbPtr = fb->top_left;

                //Update pressed buttons
                pressed = HID_PAD;
            }

            if(isA9lh && !CFG_BOOTENV && pressed == SAFE_MODE)
            {
                nandType = FIRMWARE_SYSNAND;
                firmSource = FIRMWARE_SYSNAND;

                //Flag to tell loader to init SD
                configTemp |= 1 << 5;

                //If the PIN has been verified, wait to make it easier to press the SAFE_MODE combo
                if(pinExists && !shouldLoadConfigMenu)
                {
                    while(HID_PAD & PIN_BUTTONS);
                    chrono(2);
                }
            }
            else
            {
                if(CONFIG(7) && loadSplash()) pressed = HID_PAD;

                /* If L and R/A/Select or one of the single payload buttons are pressed,
                   chainload an external payload */
                /*bool shouldLoadPayload = (pressed & SINGLE_PAYLOAD_BUTTONS) || ((pressed & BUTTON_L1) && (pressed & L_PAYLOAD_BUTTONS));

                if(shouldLoadPayload) loadPayload(pressed);*/

                if(!CONFIG(7)) loadSplash();

                //Determine if the user chose to use the SysNAND FIRM as default for a R boot
                bool useSysAsDefault = isA9lh ? CONFIG(1) : false;

                //If R is pressed, boot the non-updated NAND with the FIRM of the opposite one
                if(pressed & BUTTON_R1)
                {
                    nandType = useSysAsDefault ? FIRMWARE_EMUNAND : FIRMWARE_SYSNAND;
                    firmSource = useSysAsDefault ? FIRMWARE_SYSNAND : FIRMWARE_EMUNAND;
                }

                /* Else, boot the NAND the user set to autoboot or the opposite one, depending on L,
                   with their own FIRM */
                else
                {
                    nandType = (CONFIG(0) != !(pressed & BUTTON_L1)) ? FIRMWARE_EMUNAND : FIRMWARE_SYSNAND;
                    firmSource = nandType;
                }

                /* If we're booting emuNAND the second emuNAND is set as default and B isn't pressed,
                   or vice-versa, boot the second emuNAND */
                if(nandType != FIRMWARE_SYSNAND && (CONFIG(2) == !(pressed & BUTTON_B))) nandType = FIRMWARE_EMUNAND2;
            }
        }
    }

    //If we need to boot emuNAND, make sure it exists
    if(nandType != FIRMWARE_SYSNAND)
    {
        locateEmuNand(&emuOffset, &emuHeader, &nandType);
        if(nandType == FIRMWARE_SYSNAND) firmSource = FIRMWARE_SYSNAND;
    }

    //Same if we're using emuNAND as the FIRM source
    else if(firmSource != FIRMWARE_SYSNAND)
        locateEmuNand(&emuOffset, &emuHeader, &firmSource);

    if(!isFirmlaunch)
    {
        configTemp |= (u32)nandType | ((u32)firmSource << 2);
        writeConfig(configPath, configTemp);
    }

    u32 firmVersion = loadFirm(&firmType, firmSource);

    switch(firmType)
    {
        case NATIVE_FIRM:
            patchNativeFirm(firmVersion, nandType, emuHeader, isA9lh);
            break;
        case SAFE_FIRM:
        case NATIVE_FIRM2X:
            if(isA9lh) patch2xNativeAndSafeFirm();
            break;
        default:
            //Skip patching on unsupported O3DS AGB/TWL FIRMs
            if(isN3DS || firmVersion >= (firmType == TWL_FIRM ? 0x16 : 0xB)) patchLegacyFirm(firmType);
            break;
    }

    launchFirm(firmType);
}

static inline u32 loadFirm(FirmwareType *firmType, FirmwareSource firmSource)
{
    section = firm->section;

    const char *firmwareFiles[4] = {
        "/luma/firmware.bin",
        "/luma/firmware_twl.bin",
        "/luma/firmware_agb.bin",
        "/luma/firmware_safe.bin"
    };

    u32 firmVersion;

    if(fileRead(firm, firmwareFiles[(u32)*firmType])) firmVersion = 0xFFFFFFFF;
    else
    {
        firmVersion = firmRead(firm, (u32)*firmType);

        if(!isN3DS && *firmType == NATIVE_FIRM)
        {
            //We can't boot < 2.x SysNANDs and < 3.x EmuNANDs
            if(firmVersion < 0x18)
            {
                if(firmSource != FIRMWARE_SYSNAND || firmVersion < 9)
                    error("An old unsupported NAND has been detected.\nLuma3DS is unable to boot it");

                if(BOOTCONFIG(5, 1)) error("SAFE_MODE is not supported on 2.x FIRM");

                *firmType = NATIVE_FIRM2X;
            }

            //We can't boot a 3.x/4.x NATIVE_FIRM, load one from SD
            else if(firmVersion < 0x25)
            {
                if(!fileRead(firm, "/luma/firmware.bin") || (((u32)section[2].address >> 8) & 0xFF) != 0x68)
                    error("An old unsupported FIRM has been detected.\nCopy firmware.bin in /luma to boot");

                //No assumption regarding FIRM version
                firmVersion = 0xFFFFFFFF;
            }
        }

        decryptExeFs((u8 *)firm);
    }

    return firmVersion;
}

static inline void patchNativeFirm(u32 firmVersion, FirmwareSource nandType, u32 emuHeader, bool isA9lh)
{
    u8 *arm9Section = (u8 *)firm + section[2].offset,
       *arm11Section1 = (u8 *)firm + section[1].offset;

    if(isN3DS)
    {
        //Decrypt ARM9Bin and patch ARM9 entrypoint to skip arm9loader
        arm9Loader(arm9Section);
        firm->arm9Entry = (u8 *)0x801B01C;
    }

    //Sets the 7.x NCCH KeyX and the 6.x gamecard save data KeyY on >= 6.0 O3DS FIRMs, if not using A9LH
    else if(!isA9lh && firmVersion >= 0x29) setRSAMod0DerivedKeys();

    //Find the Process9 .code location, size and memory address
    u32 process9Size,
        process9MemAddr;
    u8 *process9Offset = getProcess9(arm9Section + 0x15000, section[2].size - 0x15000, &process9Size, &process9MemAddr);

    //Find Kernel11 SVC table and free space locations
    u8 *freeK11Space;
    u32 *arm11SvcHandler,
        *arm11ExceptionsPage;
    u32 baseK11VA;

    u32 *arm11SvcTable = getKernel11Info(arm11Section1, section[1].size, &baseK11VA, &freeK11Space, &arm11SvcHandler, &arm11ExceptionsPage);

    installMMUHook(arm11Section1, section[1].size, &freeK11Space);
    installK11MainHook(arm11Section1, section[1].size, baseK11VA, arm11SvcTable, arm11ExceptionsPage, &freeK11Space);

    //Apply signature patches
    patchSignatureChecks(process9Offset, process9Size);

    //Apply emuNAND patches
    if(nandType != FIRMWARE_SYSNAND)
    {
        u32 branchAdditive = (u32)firm + section[2].offset - (u32)section[2].address;
        patchEmuNand(arm9Section, section[2].size, process9Offset, process9Size, emuHeader, branchAdditive);
    }

    //Apply FIRM0/1 writes patches on sysNAND to protect A9LH
    else if(isA9lh) patchFirmWrites(process9Offset, process9Size);

    //Apply firmlaunch patches
    patchFirmlaunches(process9Offset, process9Size, process9MemAddr);

    //11.0 FIRM patches
    if(firmVersion >= (isN3DS ? 0x21 : 0x52))
    {
        //Apply anti-anti-DG patches
        patchTitleInstallMinVersionCheck(process9Offset, process9Size);
    }

    reimplementSvcBackdoorAndImplementCustomBackdoor(arm11SvcTable, &freeK11Space, arm11ExceptionsPage);

    //Apply UNITINFO patch
    if(DEV_OPTIONS == 1) patchUnitInfoValueSet(arm9Section, section[2].size);

    if(DEV_OPTIONS != 2)
    {
        //Kernel9/Process9 debugging
        patchArm9ExceptionHandlersInstall(arm9Section, section[2].size);
        patchSvcBreak9(arm9Section, section[2].size, (u32)section[2].address);
        patchKernel9Panic(arm9Section, section[2].size, NATIVE_FIRM);

        //Stub kernel11panic with "bkpt 65534"
        patchKernel11Panic(arm11Section1, section[1].size);
    }

    if(CONFIG(9))
    {
        patchArm11SvcAccessChecks(arm11SvcHandler);
        patchP9AccessChecks(process9Offset, process9Size);
    }
}

static inline void patchLegacyFirm(FirmwareType firmType)
{
    u8 *arm9Section = (u8 *)firm + section[3].offset;

    //On N3DS, decrypt ARM9Bin and patch ARM9 entrypoint to skip arm9loader
    if(isN3DS)
    {
        arm9Loader(arm9Section);
        firm->arm9Entry = (u8 *)0x801301C;
    }

    //Apply UNITINFO patch
    if(DEV_OPTIONS == 1) patchUnitInfoValueSet(arm9Section, section[3].size);

    if(DEV_OPTIONS != 2)
    {
        //Kernel9/Process9 debugging
        patchArm9ExceptionHandlersInstall(arm9Section, section[3].size);
        patchSvcBreak9(arm9Section, section[3].size, (u32)section[3].address);
        patchKernel9Panic(arm9Section, section[3].size, firmType);
    }

    applyLegacyFirmPatches((u8 *)firm, firmType);

    if(firmType == TWL_FIRM && CONFIG(5)) patchTwlBg((u8 *)firm + section[1].offset);
}

static inline void patch2xNativeAndSafeFirm(void)
{
    u8 *arm9Section = (u8 *)firm + section[2].offset;

    if(isN3DS)
    {
        //Decrypt ARM9Bin and patch ARM9 entrypoint to skip arm9loader
        arm9Loader(arm9Section);
        firm->arm9Entry = (u8 *)0x801B01C;

        patchFirmWrites(arm9Section, section[2].size);
    }
    else patchOldFirmWrites(arm9Section, section[2].size);

    if(DEV_OPTIONS != 2)
    {
        //Kernel9/Process9 debugging
        patchArm9ExceptionHandlersInstall(arm9Section, section[2].size);
        patchSvcBreak9(arm9Section, section[2].size, (u32)section[2].address);
    }
}

static inline void copySection0AndInjectSystemModules(FirmwareType firmType)
{
    u32 srcModuleSize,
        dstModuleSize;

    u8  *src    = (u8 *)firm + section[0].offset,
        *srcEnd = src + section[0].size,
        *dst    = section[0].address;
    for(;
        src < srcEnd; src += srcModuleSize, dst += dstModuleSize)
    {
        srcModuleSize = *(u32 *)(src + 0x104) * 0x200;
        char *moduleName = (char *)(src + 0x200);

        char fileName[30] = "/luma/sysmodules/";
        const char *ext = ".cxi";

        //Read modules from files if they exist
        u32 nameOff;
        for(nameOff = 0; nameOff < 8 && moduleName[nameOff] != 0; nameOff++);
        memcpy(fileName + 17, moduleName, nameOff);
        memcpy(fileName + 17 + nameOff, ext, 5);

        u32 fileSize = fileRead(dst, fileName);
        if(fileSize) dstModuleSize = fileSize;
        else
        {
            void *module;

            if(firmType == NATIVE_FIRM && memcmp(moduleName, "loader", 6) == 0)
            {
                module = (void *)injector;
                dstModuleSize = injector_size;
            }
            else
            {
                module = src;
                dstModuleSize = srcModuleSize;
            }

            memcpy(dst, module, dstModuleSize);
        }
    }

    // Inject Rosalina
    rosalinaModuleSize = fileRead(dst, "/luma/rosalina.cxi");
}

static inline void launchFirm(FirmwareType firmType)
{
    //If we're booting NATIVE_FIRM, section0 needs to be copied separately to inject 3ds_injector
    u32 sectionNum;
    if(firmType != SAFE_FIRM)
    {
        copySection0AndInjectSystemModules(firmType);

        const char *moduleLoadingErrors[4] = {
            "moduleAmountPatch not found",
            "modulePidPatch not found",
            "maxModuleDstPatch not found",
            "maxModuleSectionSizePatch not found"
        };

        u8 s = patchK11ModuleLoading(section[0].size, rosalinaModuleSize, (u8*) firm + section[1].offset, section[1].size);
        if(s)
        {
          fb->top_left = topLeftFbPtr;
          initScreens();

          drawString("An error has occurred:", 10, 10, COLOR_RED);
          int posY = drawString(moduleLoadingErrors[s - 1], 10, 30, COLOR_WHITE);
          drawString("Press any button to continue", 10, posY + 2 * SPACING_Y, COLOR_WHITE);

          waitInput();
        }
        sectionNum = 1;
    }
    else sectionNum = 0;

    //Copy FIRM sections to respective memory locations
    for(; sectionNum < 4 && section[sectionNum].size; sectionNum++)
        memcpy(section[sectionNum].address, (u8 *)firm + section[sectionNum].offset, section[sectionNum].size);

    //Determine the ARM11 entry to use
    vu32 *arm11;
    if(isFirmlaunch) arm11 = (u32 *)0x1FFFFFFC;
    else
    {
        deinitScreens();
        arm11 = (u32 *)0x1FFFFFF8;
    }

    //Set ARM11 kernel entrypoint
    *arm11 = (u32)firm->arm11Entry;

    flushEntireDCache(); //Ensure that all memory transfers have completed and that the data cache has been flushed
    flushEntireICache();

    //Final jump to ARM9 kernel
    ((void (*)())firm->arm9Entry)();
}
