#pragma once

#include <3ds/types.h>
#include "menu.h"

void updateN3DSMenuStatus(void);
void N3DS_ChangeClockRate(void);
void N3DS_EnableDisableL2Cache(void);

extern Menu menu_n3ds;
