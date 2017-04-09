.text
.arm
.balign 4

.global svcHandler
.type   svcHandler, %function
svcHandler:
    srsdb sp!, #0x13
    stmfd sp, {r8-r11, sp, lr}^
    sub sp, #0x18
    mrs r9, spsr
    ands r9, #0x20
    ldrneb r9, [lr, #-2]
    ldreqb r9, [lr, #-4]

    mov lr, #0              @ do stuff as if the "allow debug" flag is always set
    push {r0-r7, r12, lr}
    mov r10, #1
    strb r9, [sp, #0x58+3]  @ page end - 0xb8 + 3: svc being handled
    strb r10, [sp, #0x58+1] @ page end - 0xb8 + 1: "allow debug" flag

    @ sp = page end - 0x110
    add r0, sp, #0x110       @ page end
    blx svcHook
    mov r8, r0
    ldmfd sp, {r0-r7, r12, lr}
    cmp r8, #0
    beq _fallback            @ invalid svc, or svc 0xff (stop point)

    _handled_svc:            @ unused label, just here for formatting
        push {r0-r12, lr}
        add r0, sp, #0x148
        blx signalSvcEntry
        pop {r0-r12, lr}

        cpsie i
        blx r8
        cpsid i

        push {r0-r12, lr}
        add r0, sp, #0x148
        blx signalSvcReturn
        pop {r0-r12, lr}

        ldrb lr, [sp, #0x58+0] @ page end - 0xb8 + 0: scheduling flags
        b _fallback_end

    _fallback:
        mov r0, r9
        ldr r8, =svcFallbackHandler
        ldr r8, [r8]
        blx r8
        mov lr, #0

    _fallback_end:

    mov r8, #0
    strb r8, [sp, #0x58+3]  @ page_end - 0xb8 + 3: svc being handled
    ldr r8, [sp, #0x24]     @ page_end - 0xec: saved lr (see above) : should reload regs
    cmp r8, #0
    addeq sp, #0x24
    popne {r0-r7, r12}
    add sp, #4

    ldr r8, =officialSvcHandlerTail
    ldr r8, [r8]
    bx r8
