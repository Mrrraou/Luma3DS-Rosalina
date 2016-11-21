#pragma once

#include "kernel.h"
#include <3ds/svc.h>

extern u32 L2MMUTableFor0x40000000[256];

void constructL2TranslationTableForRosalina(void);
void mapKernelExtensionAndSetupExceptionHandlers(void);
