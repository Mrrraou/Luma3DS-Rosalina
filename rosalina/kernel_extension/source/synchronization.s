.text
.arm
.balign 4

.global coreBarrier
.type   coreBarrier, %function
coreBarrier:
    push {lr}
    bl getNumberOfCores
    ldr r1, =_barrier

    _try_loop_coreBarrier:
        ldrex r2, [r1]
        add r2, #1
        strex r3, r2, [r1]
        cmp r3, #0
        bne _try_loop_coreBarrier   @ strex failed

    mcr p15, 0, r3, c7, c10, 5      @ DMB

    _wait_loop_coreBarrier:
        ldr r3, [r1]
        cmp r3, r0
        bne _wait_loop_coreBarrier

    mov r0, #0
    str r0, [r1]
    mcr p15, 0, r0, c7, c10, 5      @ DMB

    pop {pc}

.bss
.balign 4
_barrier: .word 0
