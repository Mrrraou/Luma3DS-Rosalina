#include <3ds.h>
#include "menu.h"
#include "draw.h"
#include "memory.h"
#include "ifile.h"
#include "menus.h"

static MyThread menuThread;
static u8 ALIGN(8) menuThreadStack[THREAD_STACK_SIZE];

MyThread menuCreateThread(void)
{
    MyThread_Create(&menuThread, menuThreadMain, menuThreadStack, 0, CORE_SYSTEM);
    return menuThread;
}

void menuThreadMain(void)
{
    while(true)
    {
        if((HID_PAD & (BUTTON_L1 | BUTTON_DOWN | BUTTON_SELECT)) == (BUTTON_L1 | BUTTON_DOWN | BUTTON_SELECT))
            menuShow();

        svcSleepThread(5 * 1000 * 1000); // 5ms second
    }
}

static void menuDraw(Menu *menu, u32 selected)
{
    draw_fillFramebuffer(0);
    draw_string(menu->title, 10, 10, COLOR_TITLE);

    for(u32 i = 0; i < 15; i++)
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
    vu32 sleep_i = 0;

    u32 selected_item = 0;
    Menu *current_menu = &menu_rosalina;
    u32 previous_menus = 0;
    Menu *previous_menu[0x80];

    draw_setupFramebuffer();

    while(HID_PAD);

    while(true)
    {
        menuDraw(current_menu, selected_item);
        for(sleep_i = 0; sleep_i < 0x1800000; sleep_i++);

        while(!HID_PAD);

        if(HID_PAD & BUTTON_A)
        {
            switch(current_menu->item[selected_item].action_type)
            {
                case METHOD:
                    for(sleep_i = 0; sleep_i < 0x1800000; sleep_i++);

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
    draw_restoreFramebuffer();
    
    // ghetto sleep
    for(sleep_i = 0; sleep_i < 0x5000000; sleep_i++);
}
