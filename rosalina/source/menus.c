#include <3ds.h>
#include <stdio.h>
#include "menus.h"
#include "menu.h"
#include "draw.h"
#include "memory.h"
#include "ifile.h"
#include "utils.h"


Menu menu_rosalina = {
	"Rosalina menu",
	.items = 5,
	{
		{"Process list", METHOD, .method = &RosalinaMenu_ProcessList},
		{"b", METHOD, .method = NULL},
		{"c", METHOD, .method = NULL},
		{"d", METHOD, .method = NULL},
		{"Credits", METHOD, .method = &RosalinaMenu_ShowCredits}
	}
};

void RosalinaMenu_ShowCredits(void)
{
	draw_copyToFramebuffer(splash);
	draw_string("Rosalina - Development build", 10, 10, COLOR_TITLE);

	draw_string("Developed with memes by Mrrraou", 10, 30, COLOR_WHITE);
	draw_string(
		(	"Special thanks to:\n"
			"  The SALT team (Daz <3), TiniVi, yifanlu,\n"
			"  TuxSH, AuroraWright, #CTRDev memers,\n"
			"  3dbrew contributors\n"
		  "  (and motezazer for reading 3dbrew to me)\n\n"
			"-------------------------------------------\n"
			"Using ctrulib, devkitARM + ctrtool\n"
			"CFW based on Luma3DS (developer branch)"
		), 10, 50, COLOR_WHITE);

	draw_flushFramebuffer();

	while(!(HID_PAD & (BUTTON_A | BUTTON_B)));
}

void RosalinaMenu_ProcessList(void)
{
	draw_copyToFramebuffer(splash);
	draw_string("Process list", 10, 10, COLOR_TITLE);

	u32 rosalina_pid;
	svcGetProcessId(&rosalina_pid, 0xFFFF8001); // 0xFFFF8001 is an handle to the active process

	s32 process_count;
	u32 process_ids[18];
	svcGetProcessList(&process_count, process_ids, 18);
	process_ids[0] = 0;

	s32 i;
	for(i = 0; i < 18; i++)
	{
		if(i >= process_count)
			break;

		char process_name[9];
		process_name[8] = 0;

		Result res;
		DebugEventInfo debugInfo;

		if(process_ids[i] != rosalina_pid)
		{
			Handle debug_handle;
			svcDebugActiveProcess(&debug_handle, process_ids[i]);

			svcGetProcessDebugEvent(&debugInfo, debug_handle);
			res = svcContinueDebugEvent(debug_handle, 0b11);
			memcpy(process_name, debugInfo.process.process_name, 8);

			svcContinueDebugEvent(debug_handle, 0);
			svcCloseHandle(debug_handle);
		}
		else
			memcpy(process_name, "rosalina", 8);

		char str[255];
		sprintf(str, "%08lx %s res: %08lx", process_ids[i], process_name, res);

		draw_string(str, 30, 30 + i * SPACING_Y, COLOR_WHITE);
	}

	draw_flushFramebuffer();

	while(!(HID_PAD & (BUTTON_A | BUTTON_B)));
}
