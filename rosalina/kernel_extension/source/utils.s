.text
.arm
.balign 4

.global convertVAToPA
.type   convertVAToPA, %function
convertVAToPA:
    push {r4-r6, lr}
    mov r4, r0
    mov r5, r1
    mov r6, r2
    mov r0, r1
    mov r1, #0xf00
    orr r1, #0xff
    and r2, r0, r1
    bic r0, r1
    cmp r6, #1
    beq _convertVAToPA_write_check
    _convertVAToPA_read_check:
        mcr p15, 0, r0, c7, c8, 0    @ VA to PA translation with privileged read permission check
        b _convertVAToPA_end_check
    _convertVAToPA_write_check:
        mcr p15, 0, r0, c7, c8, 1    @ VA to PA translation with privileged write permission check
    _convertVAToPA_end_check:
    mrc p15, 0, r0, c7, c4, 0    @ read PA register
    and r3, r0, r1
    cmp r4, #0
    strne r3, [r4]
    tst r0, #1                   @ failure bit
    bic r0, r1
    addeq r0, r2
    movne r0, #0
    pop {r4-r6, pc}

.global flushEntireDataCache
.type   flushEntireDataCache, %function
flushEntireDataCache:
    mvn r1, #0              @ this is translated to a full cache flush
    ldr r12, =flushDataCacheRange
    ldr r12, [r12]
    bx r12

.global invalidateEntireInstructionCache
.type   invalidateEntireInstructionCache, %function
invalidateEntireInstructionCache:
    mvn r1, #0              @ this is translated to a full cache flush
    ldr r12, =invalidateInstructionCacheRange
    ldr r12, [r12]
    bx r12

.global KObjectMutex__Acquire
.type   KObjectMutex__Acquire, %function
KObjectMutex__Acquire:
    ldr r1, =0xFFFF9000     @ current thread addr

    ldr r1, [r1]
    ldrex r2, [r0]
    cmp r2, #0
    strexeq r3, r1, [r0]    @ store current thread
    strexne r3, r2, [r0]    @ stored thread != NULL, no change
    cmpeq r3, #0
    bxeq lr

    ldr r12, =KObjectMutex__WaitAndAcquire
    ldr r12, [r12]
    bx r12

.global KObjectMutex__Release
.type   KObjectMutex__Release, %function
KObjectMutex__Release:
    mov r1, #0
    str r1, [r0]
    ldrh r1, [r0, #4]
    cmp r1, #0
    bxle lr

    ldr r12, =KObjectMutex__ErrorOccured
    ldr r12, [r12]
    blx r12
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

_safecpy_end:

.section .rodata

.global safecpy_sz
safecpy_sz: .word _safecpy_end - safecpy

.bss
.balign 4

.global SGI0Handler
SGI0Handler: .word 0  @ see synchronization.c

.balign 4

.section .data
.balign 4

_customInterruptEventObj: .word SGI0Handler
.global customInterruptEvent
customInterruptEvent: .word _customInterruptEventObj
