.arm
.section .text.start
.align 4

.extern starbitHandler
.type 	starbitHandler, %function

.global _start
_start:
	mov r0, r4									@ r4 is the reply buffer
    ldr r12, =starbitHandler + 1				@ thumb mode
	bx r12
