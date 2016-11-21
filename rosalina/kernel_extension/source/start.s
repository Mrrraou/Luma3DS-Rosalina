.section .text.start
.balign 4
.global _start
_start:
    push {lr}

    mrc p15, 0, r1, c0, c0, 5   @ CPUID register
    and r1, #3
    cmp r1, #1
    wfene
    bne end

    ldr r1, =interruptManager
    str r0, [r1]
    blx main
    sev

    end:
    pop {pc}
