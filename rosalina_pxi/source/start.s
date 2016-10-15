.section .text.start
.align 4
.global _start
_start:
    mov r6, r0
    adr r0, _setupVram
    bl svcBackdoor
    mov r0, r6
    bl main
    b svcExitThread

_setupVram:
    ldr r0, =0x1800002D @ 18000000 8M   | vram (+ 2MB)
    mcr p15, 0, r0, c6, c6, 0
    bx lr
