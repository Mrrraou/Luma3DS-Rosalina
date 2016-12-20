.arm.little

.create "build/svcBackdoors.bin", 0
.arm

svcBackdoor:
    ; Nintendo's code
    bic   r1, sp, #0xff
    orr   r1, r1, #0xf00
    add   r1, r1, #0x28     ; get user stack.
                            ; you'll get nice crashes if an interrupt or context switch occurs during svcBackdoor
    ldr   r2, [r1]
    stmdb r2!, {sp, lr}
    mov   sp, r2
    blx   r0
    pop   {r0, r1}
    mov   sp, r0
    bx    r1

; Result svcCustomBackdoor(void *func, ... <up to 3 args>)
svcCustomBackdoor:
    push {r4, lr}
    mov r4, r0
    mov r0, r1
    mov r1, r2
    mov r2, r3
    blx r4
    pop {r4, pc}

.pool
.close
