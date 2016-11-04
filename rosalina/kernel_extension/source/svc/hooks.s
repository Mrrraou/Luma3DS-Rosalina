.arm
.section .text
.balign 4

.macro SVC_HOOK name
    .global \name\()Hook
    .type   \name\()Hook, %function
    \name\()Hook:
.endm

SVC_HOOK svcSleepThread                                             @ Test hook
    ldr r12, =svcSleepThread
    ldr pc, [r12]


.section .data
.balign 4

.macro SVC_ORIGINAL name
    .global \name\()
    \name\():
        .word 0x00000000
.endm

SVC_ORIGINAL svcSleepThread
