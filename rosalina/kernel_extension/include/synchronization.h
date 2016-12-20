#pragma once

#include "types.h"

#define MPCORE_REGS_BASE        ((u32)PA_PTR(0x17E00000))
#define MPCORE_SCU_CFG          (*(vu32 *)(MPCORE_REGS_BASE + 4))
#define MPCORE_INT_ACK          (*(vu32 *)(MPCORE_REGS_BASE + 0x1C0))

#define MPCORE_GID_REGS_BASE    (MPCORE_REGS_BASE + 0x1000)
#define MPCORE_GID_SGI          (*(vu32 *)(MPCORE_GID_REGS_BASE + 0xF00))

typedef void (*SGI0Callback_t)(u32, u32);

// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360f/CCHDIFIJ.html
void executeFunctionOnCores(SGI0Callback_t func, u8 targetList, u8 targetListFilter, u32 parameter);

u32 getNumberOfCores(void);
