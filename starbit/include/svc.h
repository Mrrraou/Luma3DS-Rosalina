#pragma once

#include "types.h"


Result svcExitThread(void);
Result svcSleepThread(u64 nanoseconds);
Result svcBreak(u32 breakType);
u32 svcBackdoor(void *func, ...);
