#pragma once

#include <3ds/types.h>

typedef u32(*backdoor_fn)(u32 arg0, u32 arg1);
u32 svc_7b(void* entry_fn, ...); // can pass up to two arguments to entry_fn(...)

void *convertVAToPA(const void *VA);