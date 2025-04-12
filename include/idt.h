// idt.h - Interrupt Descriptor Table structures and functions
#ifndef IDT_H
#define IDT_H

#include "common.h"

struct idt_entry
{
    uint16_t base_low;
    uint16_t segment_selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint32_t base;
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

#endif