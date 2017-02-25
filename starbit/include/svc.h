#pragma once

#include "types.h"


Result svcExitThread(void);
Result svcSleepThread(s64 nanoseconds);
Result svcBreak(u32 breakType);
Result svcInvalidateProcessDataCache(Handle process, void *addr, u32 size);
Result svcFlushProcessDataCache(Handle process, void *addr, u32 size);
void svcBackdoor(void *func);

void invalidateInstructionCacheRange(void *addr, u32 size);
