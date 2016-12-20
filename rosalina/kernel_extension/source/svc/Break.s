.text
.arm
.balign 4

.global BreakHook
.type   BreakHook, %function
BreakHook:
    bkpt 0xffff
