#pragma once

#include <3ds/types.h>

#define SDK_VERSION 0x70200C8


/// Initializes the service API.
Result srvSysInit(void);

/// Exits the service API.
Result srvSysExit(void);

/// Initializes FSUSER. Requires FSREG.
void fsSysInit(void);
