#pragma once

#define PA_FROM_VA_PTR(addr)    PA_PTR(convertVAToPA(addr))

#include "utils.h"

Result svc0x2F(void *function, ...); // custom backdoor before kernel ext. is installed (and only before!)

void *convertVAToPA(const void *VA);

u32 getNumberOfCores(void);

extern u8 kernel_extension[];
extern u32 kernel_extension_size;
