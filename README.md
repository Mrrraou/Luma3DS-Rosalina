# Luma3DS
*Noob-proof (N)3DS "Custom Firmware"*

*With Rosalina (∩ ͡° ͜ʖ ͡°)⊃━☆ﾟ*

N.B.: `rosalina` needs to be built separately (then put `rosalina.cxi` in the `luma` folder). `starbit` needs a rework and is disabled.

## Rosalina features

* A menu which can be opened at any time (combo: **L+Down+B**), featuring:
    * A process list submenu
    * A FS/SM patch submenu
    * A N3DS Clock+L2 submenu (hidden on O3DS)
    * A screenshot action
    * A display of the remaining battery percentage
* A "kernel extension" extending the features of Kernel11 ( ͡° ͜ʖ ͡°):
    * New features for existing SVCs, for example:
        * New types for `svcGetProcessInfo`: 0x10000 to get a process's name, 0x10001 to get a process's title ID
        * New types for `svcGetSystemInfo`: 0x10000 for CFW info, 0x10001 for N3DS-related info, etc.
        * Some other additions not listed here
    * New SVCs:
        * 0x80 for `Result svcCustomBackdoor(void *fn, ... /* up to 3 args */)`. This uses the supervisor stack and thus doesn't crash when an interrupt is handled
        * 0x2E for the now deprecated `svcGetCFWInfo`
    * IPC monitoring:
        * Perfect-compatibilty "language emulation"
    * Debug features:
        * SVC permission checks don't exist anymore
        * Everything behaves as if the "Allow debug" kernel flags was always set, and `svcKernelSetState` as well as the official debug handlers always believe that the unit is a development one. This is needed for the below item
        * Numerous fixes of Luma3DS's fatal exception handlers. Moreover, they are now only used either on privileged-mode crashes or when there is no preferred alternative, that are namely: KDebug based-debugging, or user-defined exception handlers.
    * New memory mapping: `PA 00000000..30000000 -> VA 80000000..B0000000 [ Priv: RWX, User: RWX ] [ Shared, Strongly Ordered ]`

Because of memory issues, `ErrDisp` is not launched; `err:f` is still to be reimplemented.

---

## What it is

**Luma3DS** is a program to patch the system software of (New) Nintendo 3DS handheld consoles "on the fly", adding features (such as per-game language settings and debugging capabilities for developers) and removing restrictions enforced by Nintendo (such as the region lock).
It also allows you to run unauthorized ("homebrew") content by removing signature checks.  
To use it, you will need a console capable of running homebrew software on the ARM9 processor. We recommend [Plailect's guide](https://3ds.guide/) for details on how to get your system ready.

---

## Compiling

First you need to clone the repository recursively with: `git clone --recursive https://github.com/AuroraWright/Luma3DS.git`  
To compile, you'll need [armips](https://github.com/Kingcom/armips) and a build of a recent commit of [makerom](https://github.com/profi200/Project_CTR) added to your PATH.  
For now, you'll also need to update your [libctru](https://github.com/smealum/ctrulib) install, building from the latest commit.  
For your convenience, here are [Windows](http://www91.zippyshare.com/v/ePGpjk9r/file.html) and [Linux](https://mega.nz/#!uQ1T1IAD!Q91O0e12LXKiaXh_YjXD3D5m8_W3FuMI-hEa6KVMRDQ) builds of armips (thanks to who compiled them!).  
Finally just run `make` and everything should work!  
You can find the compiled files in the `out` folder.

---

## Setup / Usage / Features

See https://github.com/AuroraWright/Luma3DS/wiki

---

## Credits

See https://github.com/AuroraWright/Luma3DS/wiki/Credits + credits in the Rosalina menu

---

## Licensing

This software is licensed under the terms of the GPLv3.  
You can find a copy of the license in the LICENSE.txt file.
