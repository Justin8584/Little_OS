// idt.h - Interrupt Descriptor Table structures and functions
#ifndef IDT_H
#define IDT_H

#include "common.h" // For standard integer types like uint32_t

// Structure for an IDT entry (Interrupt Gate)
struct idt_entry
{
    uint16_t base_low;         // Lower 16 bits of handler function address
    uint16_t segment_selector; // Kernel segment selector (usually 0x08)
    uint8_t zero;              // Always zero
    uint8_t flags;             // Type and attributes (e.g., P, DPL, S, Type=0xE for 32-bit int gate)
    uint16_t base_high;        // Upper 16 bits of handler function address
} __attribute__((packed));     // Prevent compiler padding

// Structure for the IDT pointer (used with lidt instruction)
struct idt_ptr
{
    uint16_t limit; // Size of IDT in bytes minus 1
    uint32_t base;  // Linear address of the first IDT entry
} __attribute__((packed));

// Declare ISR stubs (implemented in assembly: idt_asm.s)
// We only declare the ones we'll use initially
extern void isr0(); // Divide by zero exception
// ... add more 'extern void isrN();' lines for other CPU exceptions if you handle them
extern void irq0(); // Timer interrupt (IRQ 0)
extern void irq1(); // Keyboard interrupt (IRQ 1)
// ... add more 'extern void irqN();' lines for other hardware interrupts

// Function to initialize the IDT and PIC
void idt_init();

#endif // IDT_H