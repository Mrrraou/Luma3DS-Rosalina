#pragma once

#include <3ds/types.h>
#include "menu.h"

void Debugger_Enable(void);
void Debugger_Disable(void);

extern bool debugger_handle_update_needed;

extern Menu menu_debugger;
