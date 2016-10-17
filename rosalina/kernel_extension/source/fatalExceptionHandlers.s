@   This file is part of Luma3DS
@   Copyright (C) 2016 Aurora Wright, TuxSH
@
@   This program is free software: you can redistribute it and/or modify
@   it under the terms of the GNU General Public License as published by
@   the Free Software Foundation, either version 3 of the License, or
@   (at your option) any later version.
@
@   This program is distributed in the hope that it will be useful,
@   but WITHOUT ANY WARRANTY; without even the implied warranty of
@   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@   GNU General Public License for more details.
@
@   You should have received a copy of the GNU General Public License
@   along with this program.  If not, see <http://www.gnu.org/licenses/>.
@
@   Additional Terms 7.b of GPLv3 applies to this file: Requiring preservation of specified
@   reasonable legal notices or author attributions in that material or in the Appropriate Legal
@   Notices displayed by works containing it.

.macro GEN_HANDLER name
    .global \name\()Handler
    .type   \name\()Handler, %function
    \name\()Handler:
        ldr sp, =(fatalExceptionStack + 0x100)
        stmfd sp!, {r0-r7}
        mov r1, #\@         @ macro expansion counter
        b _commonHandler

    .size   \name\()Handler, . - \name\()Handler
.endm

.macro GEN_VENEER name
    \name\()Veneer:
        clrex
        ldr pc, =\name\()Handler
    .pool
.endm

.text
.arm
.balign 4

.global _commonHandler
.type   _commonHandler, %function
_commonHandler:
    cpsid aif
    mrs r2, spsr
    mov r6, sp
    mrs r3, cpsr

    tst r2, #0x20
    bne noFPUInitNorSvcBreak
    sub r0, lr, #4
    stmfd sp!, {r0-r3, lr}
    bl convertVAToPA
    cmp r0, #0
    ldmfd sp!, {r0-r3, lr}
    beq noFPUInitNorSvcBreak
    ldr r4, [lr, #-4]
    cmp r1, #1
    bne noFPUInit

    lsl r4, #4
    sub r4, #0xc0000000
    cmp r4, #0x30000000
    bcs noFPUInitNorSvcBreak
    fmrx r0, fpexc
    tst r0, #0x40000000
    bne noFPUInitNorSvcBreak

    sub lr, #4
    srsfd sp!, #0x13
    ldmfd sp!, {r0-r7}          @ restore context
    cps #0x13                   @ FPU init
    stmfd sp, {r0-r3, r11-lr}^
    sub sp, #0x20
    ldr r12, =initFPU
    ldr r12, [r12]
    blx r12
    ldmfd sp, {r0-r3, r11-lr}^
    add sp, #0x20
    rfefd sp!

    noFPUInit:
    cmp r1, #2
    bne noFPUInitNorSvcBreak
    ldr r5, =#0xe12fff7f
    cmp r4, r5
    bne noFPUInitNorSvcBreak
    cps #0x13                   @ switch to supervisor mode
    cmp r10, #0
    addne sp, #0x28
    ldr r2, [sp, #0x1c]         @ implementation details of the official svc handler
    ldr r4, [sp, #0x18]
    msr cpsr_c, r3              @ restore processor mode
    tst r2, #0x20
    addne lr, r4, #2            @ adjust address for later
    moveq lr, r4

    noFPUInitNorSvcBreak:
    ands r4, r2, #0xf       @ get the mode that triggered the exception
    moveq r4, #0xf          @ usr => sys
    bic r5, r3, #0xf
    orr r5, r4
    msr cpsr_c, r5          @ change processor mode
    stmfd r6!, {r8-lr}
    msr cpsr_c, r3          @ restore processor mode
    mov sp, r6

    stmfd sp!, {r2,lr}

    mrc p15,0,r4,c5,c0,0    @ dfsr
    mrc p15,0,r5,c5,c0,1    @ ifsr
    mrc p15,0,r6,c6,c0,0    @ far
    fmrx r7, fpexc
    fmrx r8, fpinst
    fmrx r9, fpinst2

    stmfd sp!, {r4-r9}      @ it's a bit of a mess, but we will fix that later
                            @ order of saved regs now: dfsr, ifsr, far, fpexc, fpinst, fpinst2, cpsr, pc + (2/4/8), r8-r14, r0-r7

    bic r3, #(1<<31)
    fmxr fpexc, r3          @ clear the VFP11 exception flag (if it's set)

    mov r0, sp
    mrc p15,0,r2,c0,c0,5    @ CPU ID register

    blx fatalExceptionHandlersMain

    mov r0, #0
    mcr p15,0,r0,c7,c14,0   @ Clean and Invalidate Entire Data Cache
    mcr p15,0,r0,c7,c10,4   @ Drain Memory Barrier
    ldr r12, =mcuReboot
    ldr r12, [r12]
    bx r12

GEN_HANDLER FIQ
GEN_HANDLER undefinedInstruction
GEN_HANDLER prefetchAbort
GEN_HANDLER dataAbort

.pool

.global fatalExceptionVeneers
fatalExceptionVeneers:
    GEN_VENEER FIQ
    GEN_VENEER undefinedInstruction
    GEN_VENEER prefetchAbort
    GEN_VENEER dataAbort
