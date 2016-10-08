#include <3ds.h>
#include "menus/n3ds.h"
#include "memory.h"
#include "menu.h"


Menu menu_n3ds = {
    "New 3DS menu",
    .items = 4,
    {
        {"Faster clock + L2 enabled mode", METHOD, .method = &N3DS_SetMode_FasterClockL2},
        {"Faster clock mode", METHOD, .method = &N3DS_SetMode_FasterClock},
        {"L2 enabled mode", METHOD, .method = &N3DS_SetMode_L2},
        {"Disable all mode", METHOD, .method = &N3DS_SetMode_None},
    }
};

void N3DS_SetMode_FasterClockL2(void)
{
	svcKernelSetState(10, 0b11, 0, 0);
}

void N3DS_SetMode_FasterClock(void)
{
	svcKernelSetState(10, 0b10, 0, 0);
}

void N3DS_SetMode_L2(void)
{
	svcKernelSetState(10, 0b01, 0, 0);
}

void N3DS_SetMode_None(void)
{
	svcKernelSetState(10, 0b00, 0, 0);
}
