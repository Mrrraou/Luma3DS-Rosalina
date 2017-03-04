.arm
.balign 4

.macro SVC_BEGIN name
	.section .text.\name, "ax", %progbits
	.global \name
	.type \name, %function
	.align 2
\name:
.endm

SVC_BEGIN svcCustomBackdoor
    svc 0x80
    bx lr

SVC_BEGIN svcConvertVAToPA
    svc 0x81
    bx lr

SVC_BEGIN svcFlushDataCacheRange
    svc 0x82
    bx lr

SVC_BEGIN svcFlushEntireDataCache
    svc 0x83
    bx lr

SVC_BEGIN svcInvalidateInstructionCacheRange
    svc 0x84
    bx lr

SVC_BEGIN svcInvalidateEntireInstructionCache
    svc 0x85
    bx lr

SVC_BEGIN svcMapProcessMemoryWithSource
    svc 0x86
    bx lr
