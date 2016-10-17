.text
.arm
.align 4

@ Those subs are courtesy of TuxSH

.global svc_7b
.type   svc_7b, %function
svc_7b:
    push {r0, r1, r2}
    mov r3, sp
    add r0, pc, #12
    svc 0x7b
    add sp, sp, #8
    ldr r0, [sp], #4
    bx lr
    cpsid aif
    ldr r2, [r3], #4
    ldmfd r3!, {r0, r1}
    push {r3, lr}
    blx r2
    pop {r3, lr}
    str r0, [r3, #-4]!
    mov r0, #0
    bx lr

.global convertVAToPA
.type   convertVAToPA, %function
convertVAToPA:
    @ needs to be executed in supervisor mode
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

.global getTTB1Address
.type   getTTB1Address, %function
getTTB1Address:
    mrc p15, 0, r0, c2, c0, 1   @ Read Translation Table Base Register 1 (the one that isn't changed on context switches)
    lsr r0, #14
    lsl r0, #14
    bx lr

.global finishMMUReconfiguration
.type   finishMMUReconfiguration, %function
finishMMUReconfiguration:
    mov r0, #0
    mcr p15, 0, r0, c7, c14, 0  @ Clean and invalidate Entire Data Cache
    mcr p15, 0, r0, c8, c7, 0   @ Invalidate unified TLB
    mcr p15, 0, r0, c7, c10, 5  @ Data memory barrier
    mcr p15, 0, r0, c7, c10, 4  @ Data synchronization barrier
    bx lr

.section .rodata

.p2align 12
.global kernel_extension
kernel_extension: .incbin "build/kernel_extension.bin"
kernel_extension_end:

.balign 4
.global kernel_extension_size
kernel_extension_size: .word kernel_extension_end - kernel_extension
