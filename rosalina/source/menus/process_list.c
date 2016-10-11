#include <3ds.h>
#include "menus/process_list.h"
#include "memory.h"
#include "draw.h"
#include "menu.h"
#include "utils.h"
#include "kernel.h"


u32 rosalina_pid;

void RosalinaMenu_ProcessList(void)
{
    svcGetProcessId(&rosalina_pid, 0xFFFF8001);
    Kernel_FetchLoadedProcesses();

    s32 selected = 0, page = 0, pagePrev = 0;

    s32 processAmount = 0x40;
    while(!processes_info[--processAmount].process && processAmount > 0);
    processAmount++;

    while(true)
    {
        if(page != pagePrev)
            draw_clearFramebuffer();
        draw_string("Process list", 10, 10, COLOR_TITLE);

        for(s32 i = 0; i < PROCESSES_PER_MENU_PAGE; i++)
        {
            ProcessInfo *info = &processes_info[page * PROCESSES_PER_MENU_PAGE + i];

            if(!info->process)
                break;

            char str[18];
            hexItoa(info->pid, str, 8, false);
            str[8] = ' ';
            memcpy(str + 9, info->name, 8);
            str[17] = 0;

            draw_string(str, 30, 30 + i * SPACING_Y, COLOR_WHITE);
            draw_character(page * PROCESSES_PER_MENU_PAGE + i == selected ? '>' : ' ', 10, 30 + i * SPACING_Y, COLOR_TITLE);
        }
        draw_flushFramebuffer();

        u32 pressed = waitInput();

        if(pressed & BUTTON_B)
            break;
        else if(pressed & BUTTON_DOWN)
            selected++;
        else if(pressed & BUTTON_UP)
            selected--;
        else if(pressed & BUTTON_LEFT)
            selected -= PROCESSES_PER_MENU_PAGE;
        else if(pressed & BUTTON_RIGHT)
        {
            if(selected + PROCESSES_PER_MENU_PAGE < processAmount)
                selected += PROCESSES_PER_MENU_PAGE;
            else if((processAmount - 1) / PROCESSES_PER_MENU_PAGE == page)
                selected %= PROCESSES_PER_MENU_PAGE;
            else
                selected = processAmount - 1;
        }

        if(selected < 0)
        {
            s32 i = 0x40;
            while(!processes_info[--i].process && i > 0);
            selected = i;
        }
        else if(selected >= processAmount)
            selected = 0;

        pagePrev = page;
        page = selected / PROCESSES_PER_MENU_PAGE;
    }
}
