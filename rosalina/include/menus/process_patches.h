#pragma once

#include <3ds/types.h>
#include "menu.h"

void ProcessPatches_PatchSM(void);
void ProcessPatches_PatchFS(void);
void ProcessPatches_PatchFS_NoDisplay(void);

extern Menu menu_process_patches;
