.arm
.balign 4

.macro SVC_BEGIN name
    .section .text.\name, "ax", %progbits
    .global \name
    .type \name, %function
    .align 2
    .cfi_startproc
\name:
.endm

.macro SVC_END
    .cfi_endproc
.endm

SVC_BEGIN svcCustomBackdoor
    svc 0x80
    bx lr
SVC_END

SVC_BEGIN svcConvertVAToPA
    str r0, [sp, #-4]!
    svc 0x81
    ldr r2, [sp], #4
    cmp r2, #0
    strne r1, [r2]
    bx lr
SVC_END

SVC_BEGIN svcFlushDataCacheRange
    svc 0x82
    bx lr
SVC_END

SVC_BEGIN svcFlushEntireDataCache
    svc 0x83
    bx lr
SVC_END

SVC_BEGIN svcInvalidateInstructionCacheRange
    svc 0x84
    bx lr
SVC_END

SVC_BEGIN svcInvalidateEntireInstructionCache
    svc 0x85
    bx lr
SVC_END

SVC_BEGIN svcMapProcessMemoryWithSource
    svc 0x86
    bx lr
SVC_END
