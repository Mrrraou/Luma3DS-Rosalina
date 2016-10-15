.arm.little

value_r0 equ 0x01FF9FF4
value_r1 equ 0x01FF9FF8
;thread_c equ 0x01FFA000
thread_c equ 0x08001000
thread_s equ 0x01FF9000

.create "build/createCustomThread.bin", 0
.arm
    mov r6, r0

    mov r7, lr
    adr r0, setupMPU
    swi 0x7B
    mov lr, r7

    ldr r1, =thread_c   ; threadMain
    mov r2, r0          ; arg
    ldr r3, =thread_s   ; stackTop
    mov r0, #0x2F       ; priority
    ldr r4, =-2         ; processorId
    swi 0x08            ; svcCreateThread(0, thread_m, ret_addr, thread_s, 0x2F, -2)

    return:
        ldr r0, =value_r0
        ldr r0, [r0]
        ldr r1, =value_r1
        ldr r1, [r1]
        bx r6           ; return to the original code

    setupMPU:
        ldr r0, =0x33333333 ; rw to all memory regions for everyone
        mcr p15, 0, r0, c5, c0, 2 ; write data access
        mcr p15, 0, r0, c5, c0, 3 ; write instruction access
        bx lr

.pool
.close
