; idt_asm.s - Assembly ISR/IRQ stubs and IDT load function (Corrected)

; Declare C handler functions used by the stubs
extern isr_handler ; C handler for exceptions
extern irq_handler ; C handler for hardware interrupts

; Declare the functions we provide
global idt_load     ; Function to load IDT register (lidt)
; Declare ISR/IRQ stubs so they are globally visible to C (in idt.c)
global isr0         ; Example: Divide by zero
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
isr%1:
    cli             ; Disable interrupts immediately
    push byte 0     ; Push a dummy error code to maintain stack consistency
    push byte %1    ; Push the interrupt number
    jmp isr_common_stub ; Jump to the common handler code
%endmacro

; Common ISR stub macro (error code IS pushed by CPU)
; %1: Interrupt number
%macro ISR_ERRCODE 1
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
irq%1:
    cli             ; Disable interrupts immediately
    push byte 0     ; Push a dummy error code (IRQs don't have CPU error codes)
    push byte %2    ; Push the interrupt number (IRQ number + 32)
    jmp irq_common_stub ; Jump to the common hardware interrupt handler code
%endmacro

; --- Define the actual ISR stubs using the macros ---
ISR_NOERRCODE 0     ; ISR 0: Divide by zero exception
; ISR_NOERRCODE 1   ; ISR 1: Debug exception
; ... Add more ISR stubs for exceptions 2-31 as needed
; Note: Some exceptions push an error code (e.g., 8, 10-14, 17, 30), use ISR_ERRCODE for those

; --- Define the actual IRQ stubs using the macros ---
; Remember the second argument is ISR number (IRQ + 0x20 = IRQ + 32)
IRQ 0, 32           ; IRQ 0: Programmable Interval Timer (PIT)
IRQ 1, 33           ; IRQ 1: Keyboard controller
; IRQ 2, 34         ; IRQ 2: Cascade (used by PICs, usually not handled directly)
; ... Add more IRQ stubs for 3-15 as needed


; --- Common stub code (shared by all ISRs) ---
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

    ; Stack pointer (ESP) now points to the registers_t structure
    ; Pass ESP as the argument (a pointer) to the C handler
    push esp
    call isr_handler ; Call the main C handler function, passing pointer to register state
    add esp, 4      ; <<< CORRECTED: Clean up 4-byte pointer argument from stack

    pop ebx         ; Restore original data segment selector (popped into EBX)
    mov ds, bx      ; Restore data segments
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa            ; Pop all general purpose registers back
    add esp, 8      ; Clean up the pushed error code (or dummy) and interrupt number
    iret            ; Return from interrupt (restores CS, EIP, EFLAGS, optionally SS, ESP)


; --- Common stub code (shared by all IRQs) ---
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
    push esp
    call irq_handler ; Call the main C IRQ handler function
    add esp, 4      ; <<< CORRECTED: Clean up 4-byte pointer argument from stack

    pop ebx         ; Restore original data segment selector
    mov ds, bx      ; Restore data segments
    mov es, bx
    mov fs, bx
    mov gs, bx

    popa            ; Pop all general purpose registers back
    add esp, 8      ; Clean up the pushed error code (dummy) and interrupt number
    iret            ; Return from interrupt