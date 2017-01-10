#include <3ds.h>
#include "menus.h"
#include "menu.h"
#include "draw.h"
#include "menus/process_list.h"
#include "menus/process_patches.h"
#include "menus/n3ds.h"
#include "ifile.h"
#include "utils.h"
#include "memory.h"


Menu menu_rosalina = {
    "Rosalina menu",
    .items = 5,
    {
        {"Process list", METHOD, .method = &RosalinaMenu_ProcessList},
        {"Process patches menu...", MENU, .menu = &menu_process_patches},
        {"Take screenshot (slow!)", METHOD, .method = &RosalinaMenu_TakeScreenShot},
        {"New 3DS menu...", MENU, .menu = &menu_n3ds},
        {"Credits", METHOD, .method = &RosalinaMenu_ShowCredits}
    }
};

void RosalinaMenu_ShowCredits(void)
{
    draw_string("Rosalina - Development build", 10, 10, COLOR_TITLE);

    draw_string("Developed with memes by Mrrraou", 10, 30, COLOR_WHITE);
    draw_string(
        (
            "Special thanks to:\n"
            "  The SALT team (Daz <3), TiniVi, yifanlu,\n"
            "  TuxSH, AuroraWright, #CTRDev memers,\n"
            "  3dbrew contributors\n"
            "  (and motezazer for reading 3dbrew to me)\n\n"
            "-------------------------------------------\n"
            "Using ctrulib, devkitARM + ctrtool\n"
            "CFW based on Luma3DS (developer branch)"
        ), 10, 50, COLOR_WHITE);

    draw_flushFramebuffer();

    while(!(waitInput() & BUTTON_B));
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
    while(!(waitInput() & BUTTON_B));

#undef TRY
}
