.text
.arm
.balign 4

.global svc_7b
.type   svc_7b, %function
@ The callback can take up to 3 args. Itsreturn value is discarded
svc_7b:
    push {r0-r3}
    adr r0, _svc_7b_kernel_entry
    svc 0x7b
    add sp, #16
    bx lr

    _svc_7b_kernel_entry:
        cpsid aif                   @ Disable interrupts

        add sp, #8
        ldr r12, [sp], #4
        ldmfd sp, {r0-r2}
        sub sp, #12

        push {lr}
        blx r12
        pop {pc}


.global svc_7b_interrupts_enabled
.type   svc_7b_interrupts_enabled, %function
svc_7b_interrupts_enabled:
    push {r0-r3}
    adr r0, _svc_7b_interrupts_enabled_kernel_entry
    svc 0x7b
    add sp, #16
    bx lr

    _svc_7b_interrupts_enabled_kernel_entry:
        cpsid aif
        ldmfd sp!, {r3,lr}
        ldr r12, [sp], #4
        ldmfd sp!, {r0-r2}
        mov sp, r3

        push {lr}
        cpsie aif
        blx r12
        pop {pc}                @ This will skip the tail of svcBackdoor, but it doesn't matter since we're doing its work

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


.global getNumberOfCores
.type   getNumberOfCores, %function
getNumberOfCores:
    ldr r0, =(0x17e00004 | 1 << 31)
    ldr r0, [r0]
    and r0, #3
    add r0, #1
    bx lr

.global flushEntireCaches
.type   flushEntireCaches, %function
flushEntireCaches:
    push {lr}

    bl getNumberOfCores
    cmp r0, #4                          @ are we on N3DS?
    bne internalCachesFlush
    ldr r0, =(0x17e10100 | 1 << 31)
    ldr r0, [r0]
    tst r0, #1                          @ is the L2C enabled?
    beq internalCachesFlush

    ldr r0, =0xffff
    ldr r1, =(0x17e10730 | 1 << 31)
    str r0, [r1, #0xcc]                @ clean and invalidate by way

    _L2C_sync:
        ldr r0, [r1]                   @ L2C cache sync register
        tst r0, #1
        bne _L2C_sync

    internalCachesFlush:
    mov r0, #0
    mcr p15, 0, r0, c7, c14, 0      @ clean and invalidate the entire DCache
    mcr p15, 0, r0, c7, c10, 5      @ data memory barrier
    mcr p15, 0, r0, c7, c5,  0      @ invalidate the entire ICache & branch target cache
    mcr p15, 0, r0, c7, c10, 4      @ data synchronization barrier

    pop {pc}
