.text
.arm
.balign 4

.macro GEN_GETINFO_WRAPPER, name
    .global Get\name\()InfoHookWrapper
    .type   Get\name\()InfoHookWrapper, %function
    Get\name\()InfoHookWrapper:
        push {r12, lr}
        sub sp, #8
        mov r0, sp
        blx Get\name\()InfoHook
        pop {r1, r2, r12, pc}
.endm

GEN_GETINFO_WRAPPER System
GEN_GETINFO_WRAPPER Process
GEN_GETINFO_WRAPPER Thread

.macro GEN_OUT1_WRAPPER, name
    .global \name\()Wrapper
    .type   \name\()Wrapper, %function
    \name\()Wrapper:
        push {lr}
        sub sp, #4
        mov r0, sp
        blx \name
        pop {r1, pc}
.endm

GEN_OUT1_WRAPPER ConnectToPortHook
