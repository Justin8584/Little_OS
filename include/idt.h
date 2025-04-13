// idt.h - Interrupt Descriptor Table structures and functions
#ifndef IDT_H
#define IDT_H

#include "common.h" // For registers_t, uintN_t

// --- Structures ---

// Represents an IDT entry (gate descriptor)
struct idt_entry
{
    uint16_t base_low;         // Lower 16 bits of handler function address
    uint16_t segment_selector; // Kernel segment selector (usually 0x08)
    uint8_t zero;              // Always zero
    uint8_t flags;             // Type and attributes (e.g., 0x8E = 32-bit Int Gate, P=1, DPL=0)
    uint16_t base_high;        // Upper 16 bits of handler function address
} __attribute__((packed));     // Prevent compiler padding

// Structure for the IDTR register (used with lidt instruction)
struct idt_ptr
{
    uint16_t limit; // Size of IDT table - 1
    uint32_t base;  // Linear base address of IDT table
} __attribute__((packed));

// --- External Assembly Functions ---
// Stubs implemented in idt_asm.s

// CPU Exceptions (ISRs 0-31) - Declare the ones we'll handle
extern void isr0();  // Divide by zero
extern void isr6();  // Invalid Opcode (#UD)
extern void isr8();  // Double Fault (#DF) - pushes error code 0
extern void isr13(); // General Protection Fault (#GP) - pushes error code

// Hardware Interrupts (IRQs 0-15 -> ISRs 32-47) - Declare the ones we'll handle
extern void irq0(); // Timer (IRQ 0)
extern void irq1(); // Keyboard (IRQ 1)
// ... add more 'extern void irqN();' lines if you handle other hardware interrupts

// Function to load the IDT register (implemented in idt_asm.s)
extern void idt_load(struct idt_ptr *idt_p_addr);

// --- C Functions ---

// Function to initialize the IDT and PIC
void idt_init();

#endif // IDT_H