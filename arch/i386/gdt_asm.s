; gdt_asm.s - Assembly function to load GDT and segment registers

global gdt_flush ; Make visible to C

; Define segment selector offsets based on gdt.c
KERNEL_CODE_SELECTOR equ 0x08 ; Index 1 * 8 bytes/entry
KERNEL_DATA_SELECTOR equ 0x10 ; Index 2 * 8 bytes/entry

section .text
gdt_flush:
    ; Argument: pointer to gdt_ptr structure is at [esp+4]
    mov eax, [esp+4]    ; Get address of gdt_ptr
    lgdt [eax]          ; Load the GDT Register (lgdt)

    ; Reload segment registers to use the new GDT
    ; Use a far jump to reload CS (Code Segment)
    ; jmp offset:absolute_address
    jmp KERNEL_CODE_SELECTOR:.reload_segments

.reload_segments:
    ; Reload data segment registers (DS, ES, FS, GS, SS)
    mov ax, KERNEL_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax          ; Stack Segment also uses kernel data segment
    ret                 ; Return to caller (gdt_init in C)