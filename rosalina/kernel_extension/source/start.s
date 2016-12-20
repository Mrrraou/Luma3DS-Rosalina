.section .text.start
.balign 4
.global _start
_start:
    push {r4, lr}

    mrc p15, 0, r4, c0, c0, 5   @ CPUID register
    and r4, #3
    cmp r4, #1
    beq _core1_only

    _waitLoop:
        wfe
        ldr r0, =_setupFinished
        ldr r0, [r0]
        cmp r0, #0
        beq _waitLoop
    b end

    _core1_only:
        blx main
        ldr r0, =_setupFinished
        str r4, [r0]
        sev

    end:
    pop {r4, pc}

.bss
.balign 4
_setupFinished: .word 0
