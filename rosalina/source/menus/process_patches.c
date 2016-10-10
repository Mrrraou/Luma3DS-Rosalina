#include <3ds.h>
#include "menus/process_patches.h"
#include "memory.h"
#include "kernel.h"
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
		{0xE1A01006, 0x0},	// mov r1, r6		@ +0x0
		{0xE1A00005, 0x4},	// mov r0, r5		@ +0x4
		{0xE3500000, 0xC},	// cmp r0, #0		@ +0xC
		{0xE2850004, 0x18}	// add r0, r5, #4	@ +0x18
	},
	.patchItems = 4,
	.patch = {
		{0xE320F000, 0x8},	// nop				@ +0x8
		{0xE320F000, 0xC},	// nop				@ +0xC
		{0xE320F000, 0x10},	// nop				@ +0x10
		{0xE320F000, 0x14}	// nop				@ +0x14
	}
};

const u8 fsArchiveCheckPattern[] = {0x18, 0x46,		// mov r0, r3
									0x81, 0x34};	// adds r4, #0x81
const u8 fsArchiveCheckPatch[] 	 = {0x01, 0x20, 	// mov r0, #1
									0x70, 0x47};	// bx lr

u32* PatchProcessByName(char *name, DestructuredPatch *patch, u32 addr, u32 size)
{
	Kernel_FetchLoadedProcesses();

	ProcessInfo *processInfo;
	for(u32 i = 0; i < sizeof(processes_info) / sizeof(ProcessInfo); i++)
	{
		if(strncmp(processes_info[i].name, name, 8) == 0)
		{
			processInfo = &processes_info[i];
			break;
		}
	}

	if(!processInfo)
		return NULL;

	Result res;
	Handle processHandle;
	if(R_FAILED(res = svcOpenProcess(&processHandle, processInfo->pid)))
		return NULL;

	if(R_FAILED(res = svcMapProcessMemory(processHandle, addr, addr + size)))
		return NULL;

	u32 *ret = DestructuredPatch_FindAndApply(patch, (u32*)addr, size);

	svcUnmapProcessMemory(processHandle, addr, addr + size);
	return ret;
}

void ProcessPatches_PatchSM(void)
{
	draw_clearFramebuffer();
	draw_flushFramebuffer();

	if(PatchProcessByName("sm", &smServiceCheckPatch, 0x100000, 0x5000))
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

	bool success = false;
	Kernel_FetchLoadedProcesses();

	ProcessInfo *processInfo;
	for(u32 i = 0; i < sizeof(processes_info) / sizeof(ProcessInfo); i++)
	{
		if(strncmp(processes_info[i].name, "fs", 8) == 0)
		{
			processInfo = &processes_info[i];
			break;
		}
	}

	if(!processInfo)
		goto END;

	Result res;
	Handle processHandle;
	if(R_FAILED(res = svcOpenProcess(&processHandle, processInfo->pid)))
		goto END;

	if(R_FAILED(res = svcMapProcessMemory(processHandle, 0x100000, 0x134000)))
		goto END;

	u8 *offset = memsearch((u8*)0x100000, fsArchiveCheckPattern, 0x34000, sizeof(fsArchiveCheckPattern));
	if(!offset)
		goto END;
	offset -= 2 * 0x4; // Beginning of subroutine
	memcpy(offset, fsArchiveCheckPatch, sizeof(fsArchiveCheckPatch));

	svcUnmapProcessMemory(processHandle, 0x100000, 0x134000);
	success = true;

END:
	if(success)
		draw_string("Successfully patched FS for the archive checks.", 10, 10, COLOR_TITLE);
	else
		draw_string("Couldn't patch FS.", 10, 10, COLOR_TITLE);

	draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
}
