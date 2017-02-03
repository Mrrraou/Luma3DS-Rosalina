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

bool isExceptionFatal(u32 spsr)
{
    if((spsr & 0x1f) != 0) return true;

    KThread *thread = currentCoreContext->objectContext.currentThread;
    KProcess *curProcess = currentCoreContext->objectContext.currentProcess;

    if(thread != NULL && thread->threadLocalStorage != NULL
       && *((vu32 *)thread->threadLocalStorage + 0x10) != 0)
       return false;

    if(curProcess != NULL)
    {
        thread = KPROCESS_GET_RVALUE(curProcess, mainThread);
        if(thread != NULL && thread->threadLocalStorage != NULL
           && *((vu32 *)thread->threadLocalStorage + 0x10) != 0)
           return false;
    }

    return true;
}

void fatalExceptionHandlersMain(u32 *registerDump, u32 type, u32 cpuId)
{
    ExceptionDumpHeader dumpHeader;

    u8 codeDump[CODE_DUMP_SIZE];
    u8 *finalBuffer = (u8 *)PA_PTR(0x25000000);
    u8 *final = finalBuffer;

    dumpHeader.magic[0] = 0xDEADC0DE;
    dumpHeader.magic[1] = 0xDEADCAFE;
    dumpHeader.versionMajor = 1;
    dumpHeader.versionMinor = 2;

    dumpHeader.processor = 11;
    dumpHeader.core = cpuId & 0xF;
    dumpHeader.type = type;

    dumpHeader.registerDumpSize = REG_DUMP_SIZE;
    dumpHeader.codeDumpSize = CODE_DUMP_SIZE;

    u32 cpsr = registerDump[16];
    u32 pc   = registerDump[15] - (type < 3 ? (((cpsr & 0x20) != 0 && type == 1) ? 2 : 4) : 8);

    registerDump[15] = pc;

    //Dump code
    u8 *instr = (u8 *)pc + ((cpsr & 0x20) ? 2 : 4) - dumpHeader.codeDumpSize; //Doesn't work well on 32-bit Thumb instructions, but it isn't much of a problem
    dumpHeader.codeDumpSize = ((u32)instr & (((cpsr & 0x20) != 0) ? 2 : 4)) != 0 ? 0 : safecpy(codeDump, instr, dumpHeader.codeDumpSize);

    //Copy register dump and code dump
    final = (u8 *)(finalBuffer + sizeof(ExceptionDumpHeader));
    final += safecpy(final, registerDump, dumpHeader.registerDumpSize);
    final += safecpy(final, codeDump, dumpHeader.codeDumpSize);

    //Dump stack in place
    dumpHeader.stackDumpSize = safecpy(final, (const void *)registerDump[13], 0x1000 - (registerDump[13] & 0xFFF));
    final += dumpHeader.stackDumpSize;

    if(currentCoreContext->objectContext.currentProcess)
    {
        vu64 *additionalData = (vu64 *)final;
        KCodeSet *currentCodeSet = codeSetOfProcess(currentCoreContext->objectContext.currentProcess);
        if(currentCodeSet != NULL)
        {
            dumpHeader.additionalDataSize = 16;
            memcpy((void *)additionalData, currentCodeSet->processName, 8);
            additionalData[1] = currentCodeSet->titleId;
        }
        else dumpHeader.additionalDataSize = 0;
    }
    else dumpHeader.additionalDataSize = 0;

    dumpHeader.totalSize = sizeof(ExceptionDumpHeader) + dumpHeader.registerDumpSize + dumpHeader.codeDumpSize + dumpHeader.stackDumpSize + dumpHeader.additionalDataSize;

    //Copy header (actually optimized by the compiler)
    *(ExceptionDumpHeader *)finalBuffer = dumpHeader;
}
