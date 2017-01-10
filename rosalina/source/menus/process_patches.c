#include <3ds.h>
#include "menus/process_patches.h"
#include "memory.h"
#include "destructured_patch.h"
#include "draw.h"


Menu menu_process_patches = {
    "Process patches menu",
    .items = 2,
    {
        {"Patch SM for the service checks", METHOD, .method = &ProcessPatches_PatchSM},
        {"Patch FS for the archive checks", METHOD, .method = &ProcessPatches_PatchFS}
    }
};

static DestructuredPatch smServiceCheckPatch = {
    .matchSize = 0x18,
    .matchItems = 4,
    .match = {
        {0xE1A01006, 0x0},	// mov r1, r6        @ +0x0
        {0xE1A00005, 0x4},	// mov r0, r5        @ +0x4
        {0xE3500000, 0xC},	// cmp r0, #0        @ +0xC
        {0xE2850004, 0x18}	// add r0, r5, #4    @ +0x18
    },
    .patchItems = 4,
    .patch = {
        {0xE320F000, 0x8},  // nop               @ +0x8
        {0xE320F000, 0xC},  // nop               @ +0xC
        {0xE320F000, 0x10}, // nop               @ +0x10
        {0xE320F000, 0x14}  // nop               @ +0x14
    }
};


static DestructuredPatch fsArchiveCheckPatch = {
    .matchSize = 4,
    .matchItems = 2,
    .match = {
        {0x4618, 0x0},   // mov r0, r3      @ +0x0
        {0x3481, 0x2}    // adds r4, #0x81  @ +0x2
    },
    .patchItems = 2,
    .patch = {
        {0x2001, 0x0},  // mov r0, #1       @ +0x0
        {0x4770, 0x2}   // bx lr            @ +0x2
    }
};

u8 *PatchProcessByName(char *name, DestructuredPatch *patch, u32 addr, u32 size, bool isThumb)
{
    u32 pidList[0x40];
    s32 processCount;
    svcGetProcessList(&processCount, pidList, 0x40);
    Handle dstProcessHandle = 0;

    for(s32 i = 0; i < processCount; i++)
    {
        Handle processHandle;
        Result res = svcOpenProcess(&processHandle, pidList[i]);
        if(R_FAILED(res))
            continue;

        char procName[8] = {0};
        svcGetProcessInfo((s64 *)procName, processHandle, 0x10000);
        if(strncmp(procName, name, 8) == 0)
            dstProcessHandle = processHandle;
        else
            svcCloseHandle(processHandle);
    }

    if(dstProcessHandle == 0)
    return NULL;

    Result res;
    Handle processHandle = dstProcessHandle;

    if(R_FAILED(res = svcMapProcessMemory(processHandle, addr, addr + size)))
    return NULL;

    u8 *ret = DestructuredPatch_FindAndApply(patch, (u8*)addr, size, isThumb);

    svcUnmapProcessMemory(processHandle, addr, addr + size);
    return ret;
}

void ProcessPatches_PatchSM(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    if(PatchProcessByName("sm", &smServiceCheckPatch, 0x100000, 0x5000, false))
    draw_string("Successfully patched SM for the service checks.", 10, 10, COLOR_TITLE);
    else
    draw_string("Couldn't patch SM.", 10, 10, COLOR_TITLE);

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}

void ProcessPatches_PatchFS(void)
{
    draw_clearFramebuffer();
    draw_flushFramebuffer();

    if(PatchProcessByName("fs", &fsArchiveCheckPatch, 0x100000, 0x134000, true))
    draw_string("Successfully patched FS for the archive checks.", 10, 10, COLOR_TITLE);
    else
    draw_string("Couldn't patch FS.", 10, 10, COLOR_TITLE);

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}
