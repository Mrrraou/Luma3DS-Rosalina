#pragma once

#include "utils.h"
#include "kernel.h"

void __attribute__((noreturn)) Break(UserBreakType type, ...);
