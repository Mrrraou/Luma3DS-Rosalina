.text
.arm
.balign 4

.global svc0x2F
.type svc0x2F, %function
svc0x2F:
    @ custom backdoor before kernel ext. is installed
    svc 0x2F
    bx lr

.global svcCustomBackdoor
.type svcCustomBackdoor, %function
svcCustomBackdoor:
    svc 0x80
    bx lr

.global convertVAToPA
.type   convertVAToPA, %function
convertVAToPA:
    @ needs to be executed in supervisor mode
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

.global flushEntireICache
.type   flushEntireICache, %function
flushEntireICache:
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 5
    mcr p15, 0, r0, c7, c5,  0  @ invalidate the entire ICache & branch target cache
    mcr p15, 0, r0, c7, c10, 4  @ data synchronization barrier
    bx lr

.section .data

.p2align 12
.global kernel_extension
kernel_extension: .incbin "build/kernel_extension.bin"
.p2align 12
kernel_extension_end:

.global kernel_extension_size
kernel_extension_size: .word kernel_extension_end - kernel_extension
