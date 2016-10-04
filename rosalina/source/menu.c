#include <3ds.h>
#include <malloc.h>
#include "menu.h"
#include "draw.h"
#include "memory.h"
#include "ifile.h"
#include "menus.h"

static Thread menu_thread;
static char splash_path[] = "/luma/rosalina/splash.bin";


Thread menuCreateThread(void)
{
	u64 total;
	IFile splashfile;
	FS_Path filePath 		= {PATH_ASCII, strnlen(splash_path, PATH_MAX) + 1, splash_path},
					archivePath = {PATH_EMPTY, 1, (u8*) ""};
	Result res;

	splash = malloc(SPLASH_SIZE);
	if(R_SUCCEEDED(res = IFile_Open(&splashfile, ARCHIVE_SDMC, archivePath, filePath, FS_OPEN_READ)))
	{
		IFile_Read(&splashfile, &total, splash, SPLASH_SIZE);
		IFile_Close(&splashfile);
	}

	return menu_thread = threadCreate(menuThreadMain, 0, 0x1000, 0, CORE_SYSTEM, true);
}

void menuThreadMain(void *arg)
{
	while(1)
	{
		if((HID_PAD & (BUTTON_L1 | BUTTON_DOWN | BUTTON_SELECT)) == (BUTTON_L1 | BUTTON_DOWN | BUTTON_SELECT))
			menuShow();

		svcSleepThread(50000000); // 5/100 a second
	}
}

static void menuDraw(Menu *menu, u32 selected)
{
	draw_copyToFramebuffer(splash);
	draw_string(menu->title, 10, 10, COLOR_TITLE);

	u32 i;
	for(i = 0; i < 15; i++)
	{
		if(i >= menu->items)
			break;
		draw_string(menu->item[i].title, 30, 30 + i * SPACING_Y, COLOR_WHITE);
		if(i == selected)
			draw_character('>', 10, 30 + i * SPACING_Y, COLOR_TITLE);
	}

	draw_string("Development build", 10, SCREEN_BOT_HEIGHT - 20, COLOR_TITLE);

	draw_flushFramebuffer();
}

void menuShow(void)
{
	u32 sleep_i = 0;

	u32 selected_item = 0;
	Menu *current_menu = &menu_rosalina;
	u32 previous_menus = 0;
	Menu *previous_menu[0x80];

	draw_setupFramebuffer();

	while(HID_PAD);

	while(1)
	{
		menuDraw(current_menu, selected_item);
		for(sleep_i = 0; sleep_i < 0x5000000; sleep_i++) __asm__("mov r0, r0");

		while(!HID_PAD);

		if(HID_PAD & BUTTON_A)
		{
			switch(current_menu->item[selected_item].action_type)
			{
				case METHOD:
					for(sleep_i = 0; sleep_i < 0x5000000; sleep_i++) __asm__("mov r0, r0");

					if(current_menu->item[selected_item].method != NULL)
						current_menu->item[selected_item].method();
					break;
				case MENU:
					previous_menu[previous_menus++] = current_menu;
					current_menu = current_menu->item[selected_item].menu;
					selected_item = 0;
					break;
			}
		}
		else if(HID_PAD & BUTTON_B)
		{
			if(previous_menus > 0)
				current_menu = previous_menu[--previous_menus];
			else
				break;
		}
		else if(HID_PAD & BUTTON_DOWN)
		{
			if(++selected_item >= current_menu->items)
				selected_item = 0;
		}
		else if(HID_PAD & BUTTON_UP)
		{
			if(selected_item-- <= 0)
				selected_item = current_menu->items - 1;
		}
	}

	draw_flushFramebuffer();

	// ghetto sleep
	for(sleep_i = 0; sleep_i < 0xB000000; sleep_i++) __asm__("mov r0, r0");

	draw_restoreFramebuffer();
}
