.section .text.start
.balign 4
.global _start
_start:
    push {lr}

    bl flushEntireCaches        @ in case there's dirty data corresponding to the RW dsp&axiwram mapping
    bl coreBarrier

    mrc p15, 0, r0, c0, c0, 5   @ CPUID register
    and r0, #3
    cmp r0, #1
    bne end

    blx main

    end:
    bl coreBarrier
    bl flushEntireCaches        @ make sure our changes are seen 

    pop {pc}
