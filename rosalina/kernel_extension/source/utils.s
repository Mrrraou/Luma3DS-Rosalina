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

.global atomicStore32
.type   atomicStore32, %function
atomicStore32:
    ldrex r2, [r0]
    strex r2, r1, [r0]
    cmp r2, #0
    bne atomicStore32
    bx lr

.global safecpy
.type   safecpy, %function
safecpy:
    push {r4, lr}
    mov r3, #0
    movs r12, #1

    _safecpy_loop:
        ldrb r4, [r1, r3]
        cmp r12, #0
        beq _safecpy_loop_end
        strb r4, [r0, r3]
        add r3, #1
        cmp r3, r2
        blo _safecpy_loop

    _safecpy_loop_end:
    mov r0, r3
    pop {r4, pc}

.global _safecpy_end
_safecpy_end:

.thumb

.global setR0toR3
.type   setR0toR3, %function
setR0toR3:
    bx lr

.bss
.balign 4

.global KProcessHandleTable__ToKProcess
KProcessHandleTable__ToKProcess: .word 0

.global KProcessHandleTable__ToKAutoObject
KProcessHandleTable__ToKAutoObject: .word 0

.global KSynchronizationObject__Signal
KSynchronizationObject__Signal: .word 0

.global WaitSynchronization1
WaitSynchronization1: .word 0

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

.global exceptionStackTop
exceptionStackTop: .word 0

.global cfwInfo
cfwInfo: .word 0,0,0,0

.global SGI0Handler
SGI0Handler: .word 0  @ see synchronization.c

.global kernelUsrCopyFuncsStart
kernelUsrCopyFuncsStart: .word 0

.global kernelUsrCopyFuncsEnd
kernelUsrCopyFuncsEnd: .word 0

.global isN3DS
isN3DS: .byte 0

.balign 4

.section .data
.balign 4

_customInterruptEventObj: .word SGI0Handler
.global customInterruptEvent
customInterruptEvent: .word _customInterruptEventObj
