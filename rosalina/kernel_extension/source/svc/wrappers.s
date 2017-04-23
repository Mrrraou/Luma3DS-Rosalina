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
        bl Get\name\()InfoHook
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
        bl \name
        pop {r1, pc}
.endm

GEN_OUT1_WRAPPER ConnectToPortHook
GEN_OUT1_WRAPPER convertVAToPA
GEN_OUT1_WRAPPER CopyHandle
GEN_OUT1_WRAPPER TranslateHandle

.global ControlMemoryHookWrapper
.type   ControlMemoryHookWrapper, %function
ControlMemoryHookWrapper:
    push {lr}
    sub sp, #12
    stmia sp, {r0, r4}
    add r0, sp, #8
    bl ControlMemoryHook
    ldr r1, [sp, #8]
    add sp, #12
    pop {pc}

.global ControlMemoryEx
.type   ControlMemoryEx, %function
ControlMemoryEx:
    push {lr}
    sub sp, #8
    cmp r5, #0
    movne r5, #1
    push {r0, r4, r5}
    add r0, sp, #12
    ldr r12, =ControlMemory
    ldr r12, [r12]
    blx r12
    ldr r1, [sp, #12]
    add sp, #20
    pop {pc}
