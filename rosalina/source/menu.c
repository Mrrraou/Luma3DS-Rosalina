#include <3ds.h>
#include "menu.h"
#include "draw.h"
#include "memory.h"
#include "ifile.h"
#include "menus.h"
#include "utils.h"

u32 waitInput(void)
{
    bool pressedKey = false;
    u32 key = 0;

    //Wait for no keys to be pressed
    while(HID_PAD && !terminationRequest) svcSleepThread(25 * 1000 * 1000);

    do
    {
        //Wait for a key to be pressed
        while(!HID_PAD && !terminationRequest) svcSleepThread(25 * 1000 * 1000);
        if(terminationRequest) return 0;

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

static Result _MCUHWC_GetBatteryLevel(u8 *out)
{
    #define TRY(expr) if(R_FAILED(res = (expr))) { svcCloseHandle(mcuhwcHandle); return res; }
    Result res;
    Handle mcuhwcHandle;

    TRY(srvGetServiceHandle(&mcuhwcHandle, "mcu::HWC"));

    u32 *cmdbuf = getThreadCommandBuffer();
    cmdbuf[0] = 0x50000;

    TRY(svcSendSyncRequest(mcuhwcHandle));

    *out = (u8) cmdbuf[2];

    svcCloseHandle(mcuhwcHandle);
    return cmdbuf[1];

    #undef TRY
}

static MyThread menuThread;
static u8 ALIGN(8) menuThreadStack[THREAD_STACK_SIZE];
static u8 batteryLevel = 255;

MyThread *menuCreateThread(void)
{
    MyThread_Create(&menuThread, menuThreadMain, menuThreadStack, THREAD_STACK_SIZE, 4, CORE_SYSTEM);
    return &menuThread;
}

void menuThreadMain(void)
{
    s64 dummy;
    bool isN3DS = svcGetSystemInfo(&dummy, 0x10001, 0) == 0;

    if(!isN3DS)
    {
        menu_rosalina.items--;
        for(u32 i = 3; i <= menu_rosalina.items; i++)
            menu_rosalina.item[i] = menu_rosalina.item[i+1];
    }

    while(!terminationRequest)
    {
        if((HID_PAD & (BUTTON_L1 | BUTTON_DOWN | BUTTON_SELECT)) == (BUTTON_L1 | BUTTON_DOWN | BUTTON_SELECT))
        {
            if(R_FAILED(_MCUHWC_GetBatteryLevel(&batteryLevel)))
                batteryLevel = 255;
            svcKernelSetState(0x10000, 1);
            menuShow();
            svcKernelSetState(0x10000, 0);
        }
        svcSleepThread(50 * 1000 * 1000); // 5ms
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

    if(batteryLevel != 255)
    {
        char msg[] = "000%";

        msg[0] = '0' + (char)(batteryLevel / 100);
        msg[1] = '0' + (char)((batteryLevel / 10) % 10);
        msg[2] = '0' + (char)(batteryLevel % 10);

        for(char *c = msg; *c == '0'; c++)
            *c = ' ';

        draw_string(msg, SCREEN_BOT_WIDTH - 10 - 4 * SPACING_X, SCREEN_BOT_HEIGHT - 20, COLOR_WHITE);
    }
    else
        draw_string("    ", SCREEN_BOT_WIDTH - 10 - 4 * SPACING_X, SCREEN_BOT_HEIGHT - 20, COLOR_WHITE);

    draw_string("Development build", 10, SCREEN_BOT_HEIGHT - 20, COLOR_TITLE);

    draw_flushFramebuffer();
}

void menuShow(void)
{
    u32 selected_item = 0;
    Menu *current_menu = &menu_rosalina;
    u32 previous_menus = 0;
    Menu *previous_menu[0x80];

    draw_lock();
    draw_setupFramebuffer();
    draw_clearFramebuffer();

    menuDraw(current_menu, selected_item);
    draw_unlock();

    do
    {
        u32 pressed = waitInput();

        draw_lock();

        if(pressed & BUTTON_A)
        {
            draw_clearFramebuffer();
            draw_flushFramebuffer();

            draw_unlock();

            switch(current_menu->item[selected_item].action_type)
            {
                case METHOD:
                    if(current_menu->item[selected_item].method != NULL)
                        current_menu->item[selected_item].method();
                    break;
                case MENU:
                    previous_menu[previous_menus++] = current_menu;
                    current_menu = current_menu->item[selected_item].menu;
                    selected_item = 0;
                    break;
            }

            draw_lock();
            draw_clearFramebuffer();
            draw_flushFramebuffer();
            draw_unlock();
        }
        else if(pressed & BUTTON_B)
        {
            draw_lock();
            draw_clearFramebuffer();
            draw_flushFramebuffer();
            draw_unlock();

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
        draw_unlock();
    }
    while(!terminationRequest);

    draw_lock();
    draw_flushFramebuffer();
    draw_restoreFramebuffer();
    draw_unlock();

    svcSleepThread(50 * 1000 * 1000);
}
