;
;   This file is part of Luma3DS
;   Copyright (C) 2016 Aurora Wright, TuxSH
;
;   This program is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
;   Additional Terms 7.b of GPLv3 applies to this file: Requiring preservation of specified
;   reasonable legal notices or author attributions in that material or in the Appropriate Legal
;   Notices displayed by works containing it.
;

; Code originally from Subv

.arm.little

.create "build/k11modules.bin", 0
.arm
    ; This code searches the sm module for a specific byte pattern and patches some of the instructions
    ; in the code to disable service access checks when calling srv:GetServiceHandle

    ; It also searches the fs module for archive access check code

    ; Save the registers we'll be using
    ; Register contents:
    ; r4: Pointer to a pointer to the exheader of the current NCCH
    ; r6: Constant 0
    ; SP + 4: Pointer to the memory location where the NCCH text was loaded

    ; Execute the instruction we overwrote in our detour
    ldr r0, [r4]

    ; Save the value of the register we use
    push {r0-r4}

    ldr r1, [r0, #0x20]  ; Load the .rodata address
    ldr r0, [r0, #0x24]  ; Load the size of the .rodata section
    add r0, r1, r0       ; Max bounds of the memory region

    ldr r3, [r0, #0x200]
    ldr r2, =0x00001002  ; sm
    cmp r3, r2
    beq out

    ldr r2, =0x3A767273 ; "srv:"
    loop:
        cmp r0, r1
        blo out ; Check if we didn't go past the bounds of the memory region
        ldr r3, [r1]
        cmp r3, r2
        ldreqb r3, [r1, #4]
        cmpeq r3, #0
        addne r1, #4
        bne loop

    mov r3, #0x52 ; 'R'
    strb r3, [r1, #3]

    out:
        pop {r0-r4} ; Restore the registers we used
        bx lr ; Jump back to whoever called us

    die:
        b die

.pool
.close
