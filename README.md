# General information

Commit history for the Luma3DS fork containing Rosalina.

Originally created by myself (August/September 2016) and developed only by myself until TuxSH joined later ; I then gave the project to TuxSH (January, 13rd -- however, this commit seems to have been slightly modified? doesn't matter much).

I ended up being kicked of the project during the witch hunt against my boyfriend for reasons I don't know about ; the only way I knew I was kicked was because I had received a GitLab email.


## Credits that have been removed from release (there are some leftovers in commit history)

* TiniVi, for the ARM11 firmmodule loading patch (changes the CXI loading endptr)
* SALT, that's where most of the original ideas (not after I left the project to them) came from ; Rosalina originally was an attempt at being OVERLORD, and Starbit an attempt at being Underling.
    * especially Dazzozo, for most of the ideas and for helping me with most of the problems I had (not starting ErrDisp, PA mapping with bit 31, ...)
* motezazer, for advice, support, etc. (for me, at least)
* Mrrraou, originally creating Rosalina and Starbit (it was my actual first 3DS project!)
* #CTRDev

## People that can gladly fuck off
* TuxSH, AuroraWright: trying to use me to get information from Dazzozo, and ruining our lives with their witch hunt

## Original patches

TiniVi's original patch can be found (here)[https://github.com/TiniVi/AHPCFW/commit/9d144117b604314aca48fe1633c605229bf8911f#diff-f434083360904ab69ef79235b2a2b034R51] ([mirror](https://github.com/Mrrraou/AHPCFW/commit/9d144117b604314aca48fe1633c605229bf8911f#diff-f434083360904ab69ef79235b2a2b034R51))

## My original TODO list for Rosalina
```
Do research on:
	- doing "proper" svc hooks
	- doing an rpc server (well, how to implement it exactly)
	- mapping kernel memory (0xFFFxxxxx) as RWX
		=> check the memory region 0xDFFxxxxx/0xEFFxxxxx too, which is the whole AXIWRAM mapped as RW
	- adding a custom thread in Process9 to add custom PXI commands
	- detaching a KDebug object to make the thread/process continue properly without having to retrieve all the queued debug events
		=> or alternatively, find a better way to pause/unpause a process
	- doing a gdb server

Stuff to do (maybe):
	- an ftp server
	- proper GetLumaInfo command (and other Luma-related commands) (will likely be removed)

Stuff to do for sure, and important:
	- reimplementing ErrDisp (err:f port)
	- svc hooks (and therefore hooking up into IPC communications too)
	- doing an rpc server to do stuff like installing CIAs from the network, upload and download files from SD/NAND/GW, ...
	- setting region for started games on the fly?
	- unencrypted Nintendo 3DS folder (and maybe one on GW too?)
	- move the exception handling to Rosalina
	- patch SVCs like svcMapMemoryBlock, svcControlProcessMemory, ... for mapping as RWX etc

Stuff to do for sure, but minor:
	- update Luma when everything is done and working
	- adding battery percentage on the menu

Temporary stuff I am actually doing for now:
	- taking FS:REG before loader and getting loader to get the session handle from Rosalina is not a solution (and it doesn't even work for now! the IPC is broken for some reason)

Bugs:
	- find a way to unpause the paused processes via the debug svcs```
   
# Original README

# Luma3DS
*Noob-proof (N)3DS "Custom Firmware"*

*With Rosalina (∩ ͡° ͜ʖ ͡°)⊃━☆ﾟ*

N.B.: `rosalina` needs to be built separately (then put `rosalina.cxi` in the `luma` folder). `starbit` needs a rework and is disabled.

## Rosalina features

* A menu which can be opened at any time (combo: **L+Down+Select**), featuring:
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
