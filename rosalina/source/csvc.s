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
    svc 0x81
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

SVC_BEGIN svcControlMemoryEx
    push {r0, r4, r5}
    ldr  r0, [sp, #0xC]
    ldr  r4, [sp, #0xC+0x4]
    ldr  r5, [sp, #0xC+0x8]
    svc  0x87
    pop  {r2, r4, r5}
    str  r1, [r2]
    bx   lr
SVC_END

SVC_BEGIN svcControlService
    svc 0x88
    bx lr
SVC_END

SVC_BEGIN svcCopyHandle
    str r0, [sp, #-4]!
    svc 0x89
    ldr r2, [sp], #4
    str r1, [r2]
    bx lr
SVC_END

SVC_BEGIN svcTranslateHandle
    str r0, [sp, #-4]!
    svc 0x8A
    ldr r2, [sp], #4
    str r1, [r2]
    bx lr
SVC_END
