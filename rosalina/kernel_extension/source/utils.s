.text
.arm
.balign 4

.global convertVAToPA
.type   convertVAToPA, %function
convertVAToPA:
    mov r1, #0x1000
    sub r1, #1
    and r2, r0, r1
    bic r0, r1
    mcr p15, 0, r0, c7, c8, 0    @ VA to PA translation with privileged read permission check
    mrc p15, 0, r0, c7, c4, 0    @ read PA register
    tst r0, #1                   @ failure bit
    bic r0, r1
    addeq r0, r2
    movne r0, #0
    bx lr

.global getCurrentCoreID
.type   getCurrentCoreID, %function
getCurrentCoreID:
    mrc p15, 0, R0, c0, c0, 5
    and r0, #3
    bx lr

.global enableIRQ
.type   enableIRQ, %function
enableIRQ:
    mrs r0, cpsr
    lsr r0, #7
    and r0, #1

    cpsie i
    bx lr

.global svcHandler
.type   svcHandler, %function
svcHandler:
    srsdb sp!, #0x13
    stmfd sp, {r8-r11, sp, lr}^
    sub sp, #0x18
    mrs r9, spsr
    ands r9, #0x20
    ldreqb r9, [lr, #-2]
    ldrneb r9, [lr, #-4]

    mov lr, #0              @ do stuff as if the "allow debug" flag is always set
    push {r0-r7, r12, lr}
    mov r10, #1
    strb r9, [sp, #0x58+3]  @ page end - 0xb8 + 3: svc being handled
    strb r10, [sp, #0x58+1] @ page end - 0xb8 + 1: "allow debug" flag

    add r0, sp, #0xd0       @ page end
    blx svcHook
    mov r8, r0
    ldmfd {r0-r7, r12, lr}
    cmp r8, #0
    beq fallback            @ invalid svc, or svc 0xff (stop point)

    _handled_svc:            @ unused label, just here for formatting
        cpsie i
        blx r8
        cpsid i
        ldrb lr, [sp, #0x58+0] @ page end - 0xb8 + 0: scheduling flags
        b _fallback_end

    _fallback:
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


.thumb

.global setR0toR3
.type   setR0toR3, %function
setR0toR3:
    bx lr

.bss
.balign 4

.global KProcessHandleTable__ToKProcess
KProcessHandleTable__ToKProcess: .word 0

.global svcFallbackHandler
svcFallbackHandler: .word 0

.global officialSvcHandlerTail
officialSvcHandlerTail: .word 0

.global interruptManager
interruptManager: .word 0
.global flushEntireICache
flushEntireICache: .word 0
.global flushEntireDCacheAndL2C
flushEntireDCacheAndL2C: .word 0
.global initFPU
initFPU: .word 0
.global mcuReboot
mcuReboot: .word 0
.global coreBarrier
coreBarrier: .word 0

.global cfwInfo
cfwInfo: .word 0,0,0,0

.global isN3DS
isN3DS: .byte 0

.balign 4

.section .data
.balign 4

_customInterruptEventVtable: .word SGI0Handler  @ see synchronization.c
_customInterruptEventObj: .word _customInterruptEventVtable
.global customInterruptEvent
customInterruptEvent: .word _customInterruptEventObj
