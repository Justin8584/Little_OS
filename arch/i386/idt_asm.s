; idt_asm.s - Assembly ISR/IRQ stubs and IDT load function (DEBUG MODIFIED)

; Declare C handler functions used by the stubs
extern isr_handler ; C handler for exceptions (defined in interrupts.c)
extern irq_handler ; C handler for hardware interrupts (defined in interrupts.c)

; Declare the functions we provide
global idt_load     ; Function to load IDT register (lidt)

; Declare ISR/IRQ stubs so they are globally visible to C (in idt.c/idt.h)
global isr0         ; Divide by zero
global isr6         ; Invalid Opcode
global isr8         ; Double Fault
global isr13        ; General Protection Fault
; Add 'global isrN' for other exceptions you handle

global irq0         ; Timer
global irq1         ; Keyboard
; Add 'global irqN' for other IRQs you handle

section .text

; Function to load the IDT pointer using lidt
; Expects pointer to idt_ptr struct on the stack [esp+4]
idt_load:
    mov eax, [esp+4] ; Get pointer to idt_ptr structure from stack argument
    lidt [eax]       ; Load IDT register with the structure pointed to by EAX
    ret

; --- Macros for creating ISR/IRQ stubs ---

; Common ISR stub macro (no error code pushed by CPU)
; %1: Interrupt number
%macro ISR_NOERRCODE 1
global isr%1        ; Make the stub label global
isr%1:
    cli             ; Disable interrupts immediately
    push byte 0     ; Push a dummy error code to maintain stack consistency
    push byte %1    ; Push the interrupt number
    jmp isr_common_stub ; Jump to the common handler code
%endmacro

; Common ISR stub macro (error code IS pushed by CPU)
; %1: Interrupt number
%macro ISR_ERRCODE 1
global isr%1        ; Make the stub label global
isr%1:
    cli             ; Disable interrupts immediately
    ; Error code is already pushed by the CPU
    push byte %1    ; Push the interrupt number
    jmp isr_common_stub ; Jump to the common handler code
%endmacro

; Common IRQ stub macro
; %1: IRQ number (0-15)
; %2: Interrupt number (IRQ number + 32, due to PIC remapping)
%macro IRQ 2
global irq%1        ; Make the stub label global
irq%1:
    cli             ; Disable interrupts immediately
    push byte 0     ; Push a dummy error code (IRQs don't have CPU error codes)
    push byte %2    ; Push the interrupt number (IRQ number + 32)
    jmp irq_common_stub ; Jump to the common hardware interrupt handler code
%endmacro

; --- Define the actual ISR stubs using the macros ---
ISR_NOERRCODE 0     ; ISR 0: Divide by zero exception
ISR_NOERRCODE 6     ; ISR 6: Invalid Opcode (#UD)
ISR_ERRCODE   8     ; ISR 8: Double Fault (#DF) - pushes error code 0
ISR_ERRCODE   13    ; ISR 13: General Protection Fault (#GP) - pushes error code
; ... Add more ISR stubs for exceptions 1-31 as needed

; --- Define the actual IRQ stubs using the macros ---
; Remember the second argument is ISR number (IRQ + 0x20 = IRQ + 32)
IRQ 0, 32           ; IRQ 0: Programmable Interval Timer (PIT)
IRQ 1, 33           ; IRQ 1: Keyboard controller
; ... Add more IRQ stubs for 3-15 as needed


; --- Common stub code (shared by all ISRs) ---
; DEBUG: Removed push esp / add esp, 4
isr_common_stub:
    pusha           ; Push all general purpose registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)

    mov ax, ds      ; Get current data segment selector
    push eax        ; Save it onto the stack

    ; Load kernel data segment selector into data segment registers
    ; Make sure 0x10 matches your GDT entry for kernel data
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Stack pointer (ESP) now points to the base of the registers_t structure
    ; Pass ESP as the argument (a pointer) to the C handler
    ; The C handler `isr_handler(registers_t *regs)` receives this pointer
    call isr_handler ; Call the main C handler function

    pop ebx         ; Restore original data segment selector (popped into EBX)
    mov ds, bx      ; Restore data segments
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa            ; Pop all general purpose registers back
    add esp, 8      ; Clean up the pushed error code (or dummy) and interrupt number
    iret            ; Return from interrupt (restores CS, EIP, EFLAGS, optionally SS, ESP)


; --- Common stub code (shared by all IRQs) ---
; DEBUG: Removed push esp / add esp, 4
irq_common_stub:
    pusha           ; Push all general purpose registers

    mov ax, ds      ; Save current data segment selector
    push eax

    ; Load kernel data segment selector
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Pass pointer (ESP) to registers_t structure to C handler
    ; The C handler `irq_handler(registers_t *regs)` receives this pointer
    call irq_handler ; Call the main C IRQ handler function

    pop ebx         ; Restore original data segment selector
    mov ds, bx      ; Restore data segments
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa            ; Pop all general purpose registers back
    add esp, 8      ; Clean up the pushed error code (dummy) and interrupt number
    iret            ; Return from interrupt