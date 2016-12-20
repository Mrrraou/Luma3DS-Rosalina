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

.bss
.balign 4

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

.global isN3DS
isN3DS: .byte 0

.balign 4

.section .data
.balign 4

_customInterruptEventVtable: .word SGI0Handler  @ see synchronization.c
_customInterruptEventObj: .word _customInterruptEventVtable
.global customInterruptEvent
customInterruptEvent: .word _customInterruptEventObj
