.text
.arm
.balign 4

.global Backdoor
.type   Backdoor, %function
Backdoor:
    @ Nintendo's code
    bic   r1, sp, #0xff
    orr   r1, r1, #0xf00
    add   r1, r1, #0x28     @ get user stack.

    ldr   r2, [r1]
    stmdb r2!, {sp, lr}
    mov   sp, r2            @ sp_svc = sp_usr. You'll get nice crashes if an interrupt or context switch occurs during svcBackdoor
    blx   r0
    pop   {r0, r1}
    mov   sp, r0
    bx    r1
