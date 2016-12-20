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

SVC_BEGIN svcBackdoor
    svc 0x7B
    bx lr
