/*
*   This file is part of Luma3DS
*   Copyright (C) 2016 Aurora Wright, TuxSH
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b of GPLv3 applies to this file: Requiring preservation of specified
*   reasonable legal notices or author attributions in that material or in the Appropriate Legal
*   Notices displayed by works containing it.
*/

#include "fatalExceptionHandlers.h"
#include "utils.h"
#include "kernel.h"
#include "memory.h"

#define REG_DUMP_SIZE   4 * 23
#define CODE_DUMP_SIZE  48

u32 fatalExceptionStack[0x100/4];

void fatalExceptionHandlersMain(u32 *regs, u32 type, u32 cpuId)
{
    ExceptionDumpHeader dumpHeader;

    u32 registerDump[REG_DUMP_SIZE / 4];
    u8 codeDump[CODE_DUMP_SIZE];
    u8 *finalBuffer = convertVAToPA((void *)0xE5000000) == NULL ? (u8 *)0xF5000000 : (u8 *)0xE5000000; //VA for 0x25000000
    u8 *final = finalBuffer;

    while(*(vu32 *)final == 0xDEADC0DE && *((vu32 *)final + 1) == 0xDEADCAFE);

    dumpHeader.magic[0] = 0xDEADC0DE;
    dumpHeader.magic[1] = 0xDEADCAFE;
    dumpHeader.versionMajor = 1;
    dumpHeader.versionMinor = 2;

    dumpHeader.processor = 11;
    dumpHeader.core = cpuId & 0xF;
    dumpHeader.type = type;

    dumpHeader.registerDumpSize = REG_DUMP_SIZE;
    dumpHeader.codeDumpSize = CODE_DUMP_SIZE;

    //Dump registers
    //Current order of saved regs: dfsr, ifsr, far, fpexc, fpinst, fpinst2, cpsr, pc, r8-r12, sp, lr, r0-r7
    u32 cpsr = regs[6];
    u32 pc   = regs[7] - (type < 3 ? (((cpsr & 0x20) != 0 && type == 1) ? 2 : 4) : 8);

    registerDump[15] = pc;
    registerDump[16] = cpsr;
    for(u32 i = 0; i < 6; i++) registerDump[17 + i] = regs[i];
    for(u32 i = 0; i < 7; i++) registerDump[8 + i]  = regs[8 + i];
    for(u32 i = 0; i < 8; i++) registerDump[i]      = regs[15 + i];

    //Dump code
    u8 *instr = (u8 *)pc + ((cpsr & 0x20) ? 2 : 4) - dumpHeader.codeDumpSize; //Doesn't work well on 32-bit Thumb instructions, but it isn't much of a problem
    dumpHeader.codeDumpSize = copyMemorySafely(codeDump, instr, dumpHeader.codeDumpSize, ((cpsr & 0x20) != 0) ? 2 : 4);

    //Copy register dump and code dump
    final = (u8 *)(finalBuffer + sizeof(ExceptionDumpHeader));
    final += copyMemorySafely(final, registerDump, dumpHeader.registerDumpSize, 1);
    final += copyMemorySafely(final, codeDump, dumpHeader.codeDumpSize, 1);

    //Dump stack in place
    dumpHeader.stackDumpSize = copyMemorySafely(final, (const void *)registerDump[13], 0x1000 - (registerDump[13] & 0xFFF), 1);
    final += dumpHeader.stackDumpSize;

    if(convertVAToPA((void *)0xFFFF9004) != NULL)
    {
        vu64 *additionalData = (vu64 *)final;
        dumpHeader.additionalDataSize = 16;
        KCodeSet *currentKCodeSet = KPROCESS_GET_RVALUE(*(KProcess**)0xFFFF9004, codeSet);

        memcpy((void *)additionalData, currentKCodeSet->processName, 8);
        additionalData[1] = currentKCodeSet->titleId;
    }
    else dumpHeader.additionalDataSize = 0;

    dumpHeader.totalSize = sizeof(ExceptionDumpHeader) + dumpHeader.registerDumpSize + dumpHeader.codeDumpSize + dumpHeader.stackDumpSize + dumpHeader.additionalDataSize;

    //Copy header (actually optimized by the compiler)
    *(ExceptionDumpHeader *)finalBuffer = dumpHeader;
}
