#include <3ds.h>
#include "menu.h"
#include "draw.h"
#include "memory.h"
#include "ifile.h"
#include "menus.h"

u32 waitInput(void)
{
    bool pressedKey = false;
    u32 key;

    //Wait for no keys to be pressed
    while(HID_PAD);

    do
    {
        //Wait for a key to be pressed
        while(!HID_PAD);

        key = HID_PAD;

        //Make sure it's pressed
        for(u32 i = 0x26000; i > 0; i --)
        {
            if(key != HID_PAD) break;
            if(i == 1) pressedKey = true;
        }
    }
    while(!pressedKey);

    return key;
}

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
    draw_string(menu->title, 10, 10, COLOR_TITLE);

    for(u32 i = 0; i < 15; i++)
    {
        if(i >= menu->items)
            break;
        draw_string(menu->item[i].title, 30, 30 + i * SPACING_Y, COLOR_WHITE);
        draw_character(i == selected ? '>' : ' ', 10, 30 + i * SPACING_Y, COLOR_TITLE);
    }

    draw_string("Development build", 10, SCREEN_BOT_HEIGHT - 20, COLOR_TITLE);

    draw_flushFramebuffer();
}

void menuShow(void)
{
    u32 selected_item = 0;
    Menu *current_menu = &menu_rosalina;
    u32 previous_menus = 0;
    Menu *previous_menu[0x80];

    draw_setupFramebuffer();

    draw_fillFramebuffer(0);
    menuDraw(current_menu, selected_item);

    while(true)
    {
        u32 pressed = waitInput();

        if(pressed & BUTTON_A)
        {
            draw_fillFramebuffer(0);
            draw_flushFramebuffer();

            switch(current_menu->item[selected_item].action_type)
            {
                case METHOD:
                    if(current_menu->item[selected_item].method != NULL)
                        current_menu->item[selected_item].method();
                    else waitInput();
                    break;
                case MENU:
                    previous_menu[previous_menus++] = current_menu;
                    current_menu = current_menu->item[selected_item].menu;
                    selected_item = 0;
                    break;
            }

            draw_fillFramebuffer(0);
            draw_flushFramebuffer();
        }
        else if(pressed & BUTTON_B)
        {
            draw_fillFramebuffer(0);
            draw_flushFramebuffer();

            if(previous_menus > 0)
                current_menu = previous_menu[--previous_menus];
            else
                break;
        }
        else if(pressed & BUTTON_DOWN)
        {
            if(++selected_item >= current_menu->items)
                selected_item = 0;
        }
        else if(pressed & BUTTON_UP)
        {
            if(selected_item-- <= 0)
                selected_item = current_menu->items - 1;
        }
        else
            continue;

        menuDraw(current_menu, selected_item);
    }

    draw_flushFramebuffer();
    draw_restoreFramebuffer();

    svcSleepThread(50 * 1000 * 1000);
}
