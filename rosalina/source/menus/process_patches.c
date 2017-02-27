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

static u8 *PatchProcessByName(const char *name, DestructuredPatch *patch, u32 addr, bool isThumb)
{
    u32 pidList[0x40];
    s32 processCount;
    s64 textTotalRoundedSize = 0;
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

    svcGetProcessInfo(&textTotalRoundedSize, processHandle, 0x10002); // only patch .text
    if(R_FAILED(res = svcMapProcessMemory(processHandle, addr, addr + textTotalRoundedSize)))
    return NULL;

    u8 *ret = DestructuredPatch_FindAndApply(patch, (u8*)addr, textTotalRoundedSize, isThumb);

    svcUnmapProcessMemory(processHandle, addr, addr + textTotalRoundedSize);
    return ret;
}

void ProcessPatches_PatchProcess(const char *processName, DestructuredPatch *patch, bool isThumb, const char *success, const char *failure)
{
    draw_lock();
    draw_clearFramebuffer();
    draw_flushFramebuffer();
    draw_unlock();

    u8 *res = PatchProcessByName(processName, patch, 0x100000, isThumb);

    do
    {
        draw_lock();
        draw_string(res != NULL ? success : failure, 10, 10, COLOR_WHITE);
        draw_flushFramebuffer();
        draw_unlock();
    }
    while(!(waitInput() & BUTTON_B) && !terminationRequest);
}

void ProcessPatches_PatchSM(void)
{
    ProcessPatches_PatchProcess("sm", &smServiceCheckPatch, false, "Successfully patched SM for the service checks.", "Couldn't patch SM.");
}

void ProcessPatches_PatchFS(void)
{
    ProcessPatches_PatchProcess("fs", &fsArchiveCheckPatch, true, "Successfully patched FS for the archive checks.", "Couldn't patch FS.");
}
