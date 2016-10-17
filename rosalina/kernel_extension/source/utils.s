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

.global flushCachesRange
.type   flushCachesRange, %function
flushCachesRange:
    add r1, r0, r1                      @ end address
    bic r0, #0x1f                       @ align source address to cache line size (32 bytes)

    flush_caches_range_loop:
        mcr p15, 0, r0, c7, c14, 1      @ clean and flush the line corresponding to the address r0 is holding
        mcr p15, 0, r0, c7, c5, 1
        add r0, #0x20
        cmp r0, r1
        blo flush_caches_range_loop

    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4          @ drain write buffer
    bx lr
