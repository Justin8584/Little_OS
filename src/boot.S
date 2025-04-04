// src/boot.S - Bootstrap code for ARM

.section ".text.boot"
.global _start

_start:
    // Check processor ID, halt all secondary cores
    mrc p15, 0, r0, c0, c0, 5
    and r0, r0, #3
    cmp r0, #0
    bne halt

    // Initialize stack pointer for the primary core
    ldr sp, =_start

    // Clear BSS section
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    beq kernel_main
    str r2, [r0]
    add r0, r0, #4
    b clear_bss

    // Jump to kernel_main
kernel_main:
    bl main
    b halt

halt:
    wfe
    b halt