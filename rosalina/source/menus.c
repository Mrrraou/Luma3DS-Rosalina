#include <3ds.h>
#include "menus.h"
#include "menu.h"
#include "draw.h"
#include "menus/process_list.h"
#include "menus/process_patches.h"
#include "menus/n3ds.h"
#include "menus/debugger.h"
#include "ifile.h"
#include "utils.h"
#include "memory.h"


Menu menu_rosalina = {
    "Rosalina menu",
    .items = 6,
    {
        {"Process list", METHOD, .method = &RosalinaMenu_ProcessList},
        {"Process patches menu...", MENU, .menu = &menu_process_patches},
        {"Take screenshot (slow!)", METHOD, .method = &RosalinaMenu_TakeScreenShot},
        {"New 3DS menu...", MENU, .menu = &menu_n3ds},
        {"Debugger options...", MENU, .menu = &menu_debugger},
        {"Credits", METHOD, .method = &RosalinaMenu_ShowCredits}
    }
};

void RosalinaMenu_ShowCredits(void)
{
    draw_string("Rosalina -- Luma3DS", 10, 10, COLOR_TITLE);

    draw_string("Luma3DS (c) 2016-2017 AuroraWright, TuxSH", 10, 30, COLOR_WHITE);
    draw_string(
        (
            "Special thanks for Rosalina to:\n"
            "  Dazzozo (and SALT), Bond697, yifanlu,\n"
            "  #CTRDev, motezazer, other people\n"
        ), 10, 50, COLOR_WHITE);

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B) && !terminationRequest);
}

void RosalinaMenu_TakeScreenShot(void)
{
#define TRY(expr) if(R_FAILED(res = (expr))) goto end;

    draw_restoreFramebuffer();
    draw_flushFramebuffer();

    u64 total;
    IFile file;
    u8 buf[54];

    Result res;
    TRY(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/screenshot_top.bmp"), FS_OPEN_CREATE | FS_OPEN_WRITE));
    createBitmapHeader(buf, 400, 240);
    TRY(IFile_Write(&file, &total, buf, 54, 0));

    for(u32 y = 0; y < 240; y++)
    {
        u8 *line = convertFrameBufferLine(true, y);
        TRY(IFile_Write(&file, &total, line, 3*400, 0));
    }
    TRY(IFile_Close(&file));

    TRY(IFile_Open(&file, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/screenshot_bottom.bmp"), FS_OPEN_CREATE | FS_OPEN_WRITE));
    createBitmapHeader(buf, 320, 240);
    TRY(IFile_Write(&file, &total, buf, 54, 0));

    for(u32 y = 0; y < 240; y++)
    {
        u8 *line = convertFrameBufferLine(false, y);
        TRY(IFile_Write(&file, &total, line, 3*320, 0));
    }

end:
    IFile_Close(&file);

    draw_setupFramebuffer();
    draw_clearFramebuffer();

    draw_string("Rosalina - Development build", 10, 10, COLOR_TITLE);
    if(R_FAILED(res))
    {
        char msg[] = "Failed (0x00000000)";
        hexItoa(res, msg, 8, false);
        draw_string(msg, 10, 30, COLOR_WHITE);
    }
    draw_string("Finished without any error", 10, 30, COLOR_WHITE);

    draw_flushFramebuffer();
    while(!(waitInput() & BUTTON_B) && !terminationRequest);

#undef TRY
}
