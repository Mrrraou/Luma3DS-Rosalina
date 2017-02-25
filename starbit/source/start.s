.arm
.section .text.start
.align 4

.extern starbitHandler
.type 	starbitHandler, %function

.global _start
_start: b skip_vars
.global AES__Acquire
AES__Acquire: .word 0
.global AES__Release
AES__Release: .word 0

skip_vars:
	mov r0, r4									@ r4 is the reply buffer
    ldr r12, =starbitHandler + 1				@ thumb mode
	bx r12
