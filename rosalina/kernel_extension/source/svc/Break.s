.text
.arm
.balign 4

.global Break
.type   Break, %function
Break:
    bic r0, sp, #0xf00
    bic r0, #0xff
    add r0, #0x1000 @ get page context top
    bkpt 0xffff
