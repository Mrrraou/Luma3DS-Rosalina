.text
.arm
.balign 4

.global convertVAToPA
.type   convertVAToPA, %function
convertVAToPA:
    mov r1, #0x1000
    sub r1, #1
    and r2, r0, r1
    bic r0, r1
    mcr p15, 0, r0, c7, c8, 0    @ VA to PA translation with privileged read permission check
    mrc p15, 0, r0, c7, c4, 0    @ read PA register
    tst r0, #1                   @ failure bit
    bic r0, r1
    addeq r0, r2
    movne r0, #0
    bx lr

.global getNumberOfCores
.type   getNumberOfCores, %function
getNumberOfCores:
    ldr r0, =((0x17e00000 + 4) | 1 << 31)
    ldr r0, [r0]
    and r0, #3
    add r0, #1
    bx lr

.global SGI0Handler
.type   SGI0Handler, %function
SGI0Handler:
    push {lr}

    ldr r0, =((0x17e00000 + 0x1c0) | 1<<31)   @ Interrupt Acknoledge register
    ldr r0, [r0]
    lsr r0, #10
    and r0, #3                                @ Get interrupt source

    ldr r1, =_SGI0HandlersPerCore
    ldr r12, [r1, r0, lsl #2]

    blx r12

    mov r0, #0

    pop {pc}

.global executeFunctionOnCores
.type   executeFunctionOnCores, %function
executeFunctionOnCores:
    push {r4, lr}

    mrc p15, 0, r3, c0, c0, 5   @ CPUID register
    and r3, #3
    ldr r4, =_SGI0HandlersPerCore
    str r0, [r4, r3, lsl #2]

    and r1, #0xF
    and r2, #3
    mov r0, #0
    orr r0, r1, lsl #16
    orr r0, r2, lsl #24
    ldr r1, =((0x17e00000 + 0x1000 + 0x400) | 1<<31)
    str r0, [r1]                @ Send SGI0 to all cores except the current one

    pop {r4, pc}
.bss

_SGI0HandlersPerCore: .word 0, 0, 0, 0

.section .data

_customInterruptEventVtable: .word SGI0Handler
_customInterruptEventObj: .word _customInterruptEventVtable
.global customInterruptEvent
customInterruptEvent: .word _customInterruptEventObj
