.section .text.start
.balign 4
.global _start
_start:
    push {lr}
    blx main
    pop {pc}
