#include <3ds.h>
#include "menus/n3ds.h"
#include "memory.h"
#include "menu.h"


Menu menu_n3ds = {
    "New 3DS menu",
    .items = 4,
    {
        {"Enable L2 cache", METHOD, .method = &N3DS_EnableDisableL2Cache},
        {"Set clock rate to 804MHz", METHOD, .method = &N3DS_ChangeClockRate}
    }
};

s64 clkRate = 0, L2CacheEnabled = 0;

static void updateStatus(void)
{
    svcGetProcessInfo(&clkRate, 0x10001, 0);
    svcGetProcessInfo(&L2CacheEnabled, 0x10001, 1);

    menu_n3ds.item[0].title = L2CacheEnabled ? "Disable L2 cache" : "Enable L2 cache";
    menu_n3ds.item[1].title = clkRate == 804 ? "Set clock rate to 268MHz" : "Set clock rate to 804MHz";
}

void N3DS_ChangeClockRate(void)
{
    updateStatus();

    if(clkRate == 268)
        svcKernelSetState(10, !L2CacheEnabled ? 0b10 : 0b11);
    else
        svcKernelSetState(10, !L2CacheEnabled ? 0b00 : 0b01);

    updateStatus();
}

void N3DS_EnableDisableL2Cache(void)
{
    updateStatus();

    if(!L2CacheEnabled)
        svcKernelSetState(10, clkRate == 268 ? 0b01 : 0b11);
    else
        svcKernelSetState(10, clkRate == 268 ? 0b00 : 0b10);

    updateStatus();
}
