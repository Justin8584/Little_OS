; loader.s - The initial assembly code and Multiboot header

bits 32         ; We are targeting 32-bit protected mode

; Define constants for the Multiboot header
MB_ALIGN        equ 1 << 0            ; Align loaded modules on page boundaries
MB_MEMINFO      equ 1 << 1            ; Provide memory map
MB_FLAGS        equ MB_ALIGN | MB_MEMINFO ; Our Multiboot flags
MB_MAGIC        equ 0x1BADB002        ; Must be there for GRUB to recognize
MB_CHECKSUM     equ -(MB_MAGIC + MB_FLAGS) ; Checksum (magic + flags + checksum == 0)

KERNEL_STACK_SIZE equ 4096            ; Size of the kernel stack (4KB)


; Declare external C function kmain
extern kmain

; Reserve space for the kernel stack in the BSS (uninitialized data) section
section .bss
align 4         ; Align stack to 4-byte boundary
kernel_stack_bottom:
    resb KERNEL_STACK_SIZE ; Reserve bytes for the stack
kernel_stack_top:           ; A label pointing to the top of the stack space


; The code section starts here
section .text
align 4
; Multiboot header (must be at the beginning of the kernel file)
    dd MB_MAGIC     ; Magic number
    dd MB_FLAGS     ; Flags
    dd MB_CHECKSUM  ; Checksum


; Entry point for the kernel, called by GRUB
global loader
loader:
    ; --- Set up the stack ---
    ; Point ESP to the top of our reserved stack area
    ; Remember the stack grows downwards in memory
    mov esp, kernel_stack_top

    ; --- Call C kernel ---
    ; GRUB passes Multiboot info via registers:
    ; EBX: Pointer to the Multiboot info structure
    ; EAX: Magic number (should be 0x2BADB002 if loaded by compliant bootloader)

    ; Push arguments for kmain onto the stack (in reverse order)
    push ebx        ; Push Multiboot info pointer (arg 2)
    push eax        ; Push Multiboot magic number (arg 1)

    ; Call the C kernel main function
    call kmain

    ; --- Halt if kmain returns (it shouldn't) ---
.hang:
    cli             ; Disable interrupts permanently
    hlt             ; Halt the CPU
    jmp .hang       ; Loop forever in case hlt returns (should not happen)