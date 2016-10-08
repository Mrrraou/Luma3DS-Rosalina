#include <3ds.h>
#include "menus.h"
#include "menu.h"
#include "draw.h"
#include "menus/process_list.h"


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

    waitInput();
}
