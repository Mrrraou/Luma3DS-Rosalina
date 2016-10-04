.text
.arm
.align 4

@ Those subs are courtesy of TuxSH

.global svc_7b
.type   svc_7b, %function
svc_7b:
  push {r0, r1, r2}
  mov r3, sp
  add r0, pc, #12
  svc 0x7b
  add sp, sp, #8
  ldr r0, [sp], #4
  bx lr
  cpsid aif
  ldr r2, [r3], #4
  ldmfd r3!, {r0, r1}
  push {r3, lr}
  blx r2
  pop {r3, lr}
  str r0, [r3, #-4]!
  mov r0, #0
  bx lr

.global convertVAToPA
.type   convertVAToPA, %function
convertVAToPA:
  mov r1, #0x1000
  sub r1, #1
  and r2, r0, r1
  bic r0, r1
  mcr p15, 0, r0, c7, c8, 0    @ VA to PA translation with privileged read permission check
  mrc p15, 0, r0, c7, c4, 0    @ read PA register
  tst r0, #1              @ failure bit
  bic r0, r1
  addeq r0, r2
  movne r0, #0
  bx lr
