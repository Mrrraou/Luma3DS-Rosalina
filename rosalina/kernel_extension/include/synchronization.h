#pragma once

#include "types.h"
#include "kernel.h"

//typedef void (*SGI0Callback_t)(u32, u32);

typedef KSchedulableInterruptEvent* (*SGI0Handler_t)(KBaseInterruptEvent *this, u32 interruptID);

// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/CCHDIFIJ.html
void executeFunctionOnCores(SGI0Handler_t func, u8 targetList, u8 targetListFilter);

u32 getNumberOfCores(void);
