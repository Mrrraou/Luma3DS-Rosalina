#pragma once

#include "types.h"


void FlushEntireDCache(void); // actually: "clean and flush"
void FlushDCacheRange(void *startAddress, u32 size);
void InvalidateEntireICache(void);
void InvalidateICacheRange(void *startAddress, u32 size);
