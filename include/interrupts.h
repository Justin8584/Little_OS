#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "common.h" // Include common types like registers_t

// --- C Function Declarations ---

// Declaration for the generic ISR handler (defined in interrupts.c)
void isr_handler(registers_t *regs);

// Declaration for the generic IRQ handler (defined in interrupts.c)
void irq_handler(registers_t *regs);

// Declaration for the specific keyboard handler (defined in interrupts.c)
void keyboard_handler();

// If you add other specific handlers like timer_handler(), declare them here too.

#endif // INTERRUPTS_H
