#pragma once

#include <3ds/types.h>

#define MAKE_BRANCH(src,dst)      (0xEA000000 | ((u32)((((u8 *)(dst) - (u8 *)(src)) >> 2) - 2) & 0xFFFFFF))
#define MAKE_BRANCH_LINK(src,dst) (0xEB000000 | ((u32)((((u8 *)(dst) - (u8 *)(src)) >> 2) - 2) & 0xFFFFFF))

#define CONFIG(a)        (((config >> (a + 20)) & 1) != 0)
#define MULTICONFIG(a)   ((config >> (a * 2 + 8)) & 3)
#define BOOTCONFIG(a, b) ((config >> a) & b)

#define BOOTCFG_NAND         BOOTCONFIG(0, 7)
#define BOOTCFG_FIRM         BOOTCONFIG(3, 7)
#define BOOTCFG_A9LH         BOOTCONFIG(6, 1)
#define BOOTCFG_NOFORCEFLAG  BOOTCONFIG(7, 1)

enum multiOptions
{
    DEFAULTEMU = 0,
    BRIGHTNESS,
    SPLASH,
    PIN,
    NEWCPU
};

enum singleOptions
{
    AUTOBOOTSYS = 0,
    USESYSFIRM,
    LOADEXTFIRMSANDMODULES,
    USECUSTOMPATH,
    PATCHGAMES,
    PATCHVERSTRING,
    SHOWGBABOOT,
    PATCHACCESS,
    PATCHUNITINFO
};

void patchCode(u64 progId, u16 progVer, u8 *code, u32 size, u32 text_size);
