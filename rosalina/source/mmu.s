.text
.arm
.balign 4

.global constructL2TranslationTableForRosalina
.type   constructL2TranslationTableForRosalina, %function
constructL2TranslationTableForRosalina:
    @ 4KB extended small pages: [SYS:RW USR:-- X  TYP:NORMAL SHARED OUTER NOCACHE, INNER CACHED WB WA]
    push {r4, r5, lr}

    ldr r4, =L2MMUTableFor0x40000000

    ldr r0, =_kernel_extension
    bl convertVAToPA
    mov r5, r0

    mov r0, #0
    ldr r1, =_kernel_extension_size
    ldr r2, =0x516

    _constructL2TranslationTableForRosalina_map_loop:
        add r3, r5, r0
        orr r3, r2
        str r3, [r4, r0, lsr #(12-2)]
        add r0, #0x1000
        cmp r0, r1
        blo _constructL2TranslationTableForRosalina_map_loop

    pop {r4, r5, pc}

.global mapKernelExtensionAndSetupExceptionHandlers
.type   mapKernelExtensionAndSetupExceptionHandlers, %function
mapKernelExtensionAndSetupExceptionHandlers:
    @ args: this
    push {r4-r6, lr}

    mrs r6, cpsr
    and r6, #0x1c0

    cpsid aif
    mov r4, r0

    ldr r5, [r4, #8]
    blx r5                      @ flushEntireCaches

    mrc p15, 0, r0, c2, c0, 1   @ Read Translation Table Base Register 1 (the one that isn't changed on context switches)
    lsr r0, #14
    lsl r0, #14
    orr r0, #(1 << 31)

    ldr r1, [r4, #4]
    bic r1, #(1 << 31)          @ get PA
    orr r1, #1                  @ indicates this is a L2 Page table

    add r0, #(0x40000000 >> 20 << 2)
    str r1, [r0]                @ map 1MB of memory at 0x40000000

    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4  @ DSB. Mandatory.

    mov r12, #0x40000000
    ldr r0, [r4, #-16]          @ interruptManager
    blx r12

    blx r5

    end:

    mrs r1, cpsr
    bic r1, #0x1c0
    orr r1, r6
    msr cpsr_cx, r1             @ restore interrupts if needed

    pop {r4-r6, pc}

.bss

.p2align 10
.global L2MMUTableFor0x40000000
L2MMUTableFor0x40000000: .skip 0x400

.section .data

.p2align 12
_kernel_extension: .incbin "build/kernel_extension.bin"
.p2align 12
_kernel_extension_end:

_kernel_extension_size: .word _kernel_extension_end - _kernel_extension
