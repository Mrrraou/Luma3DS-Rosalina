.arm
.align 4

.macro SVC_BEGIN name
    .section .text.\name, "ax", %progbits
    .global \name
    .type \name, %function
    .align 2
\name:
.endm

SVC_BEGIN svcExitThread
    svc 0x09
    bx lr

SVC_BEGIN svcSleepThread
    svc 0x0A
    bx lr

SVC_BEGIN svcBreak
    svc 0x3C
    bx lr

SVC_BEGIN svcInvalidateProcessDataCache
    svc 0x52
    bx lr

SVC_BEGIN svcFlushProcessDataCache
    svc 0x54
    bx lr

SVC_BEGIN svcBackdoor
    svc 0x7B
    bx lr

.global invalidateInstructionCacheRange
.type   invalidateInstructionCacheRange, %function
    push {r4, r5}
    mov r3, r0
    mov r4, r1
    adr r0, _invalidateInstructionCacheRange
    svc 0x7b
    pop {r4, r5}
    bx lr
    _invalidateInstructionCacheRange:
        mov r0, r4
        mov r1, r5
        cmp r1, #0x4000
        ldrcs r12, =0xffff0ab4
        ldrcc r12, =0xffff0ac0
        bx r12
