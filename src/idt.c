// idt.c - IDT and PIC initialization (DEBUG MODIFIED)
#include "idt.h"
#include "io.h"
#include "string.h" // For memset
#include "common.h" // For uintN_t, registers_t
#include "fb.h"     // For printing error messages (optional)
#include "serial.h" // For serial logging
#include "interrupts.h"

#define IDT_ENTRIES 256
#define KERNEL_CODE_SEGMENT 0x08      // Selector for kernel code segment from GDT
#define IDT_INTERRUPT_GATE_32BIT 0x8E // Flags: P=1, DPL=0, Type=0xE (32-bit Int Gate)

// --- Globals ---
struct idt_entry idt[IDT_ENTRIES]; // The actual IDT table
struct idt_ptr idt_p;              // Pointer structure for lidt instruction

// Defined in idt_asm.s, loads the IDTR register
extern void idt_load(struct idt_ptr *idt_p_addr);

// Function to set an IDT entry (gate)
// Defined static as it's only used internally here
static void idt_set_gate(int num, uint32_t base, uint16_t segment_selector, uint8_t flags)
{
    if (num < 0 || num >= IDT_ENTRIES)
        return; // Basic bounds check

    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].segment_selector = segment_selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

// --- PIC Remapping (Essential!) ---
// Copied here for completeness, ensure it's not duplicated if defined elsewhere (e.g., pic.c)
#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20 /* End-of-interrupt command code */

#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

// Remaps the PIC controllers
static void pic_remap(int offset1, int offset2)
{
    unsigned char a1, a2;

    a1 = inb(PIC1_DATA); // Save masks
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // Starts the initialization sequence (in cascade mode)
    // io_wait(); // Small delay may be needed on some hardware, usually not in QEMU
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    // io_wait();
    outb(PIC1_DATA, offset1); // ICW2: Master PIC vector offset
    // io_wait();
    outb(PIC2_DATA, offset2); // ICW2: Slave PIC vector offset
    // io_wait();
    outb(PIC1_DATA, 4); // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    // io_wait();
    outb(PIC2_DATA, 2); // ICW3: tell Slave PIC its cascade identity (0000 0010)
    // io_wait();

    outb(PIC1_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    // io_wait();
    outb(PIC2_DATA, ICW4_8086);
    // io_wait();

    outb(PIC1_DATA, a1); // Restore saved masks
    outb(PIC2_DATA, a2);

    serial_printf("PIC Remapped: Master offset 0x%x, Slave offset 0x%x\n", offset1, offset2);
}

// Send End-of-Interrupt signal (called by C IRQ handler)
void pic_send_eoi(unsigned char irq)
{
    if (irq >= 8) // If IRQ is from Slave PIC (IRQ 8-15)
        outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI); // Always send EOI to Master PIC
}

// Mask/Unmask PIC IRQs (Needed if you have timer.c, keyboard.c)
void pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);
}

void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    if (irq < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// --- IDT Initialization ---
void idt_init()
{
    serial_printf("Setting up IDT...\n");
    // Set up the IDT pointer
    idt_p.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idt_p.base = (uint32_t)&idt;

    // Zero out the IDT table
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);

    // Remap the PIC controllers to avoid conflicts with CPU exceptions
    // IRQs 0-7 map to ISRs 32-39 (0x20-0x27)
    // IRQs 8-15 map to ISRs 40-47 (0x28-0x2F)
    pic_remap(0x20, 0x28);

    // --- Set up ISR Gates (CPU Exceptions) ---
    serial_printf("Setting ISR gates...\n");
    idt_set_gate(0, (uint32_t)isr0, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT);
    idt_set_gate(6, (uint32_t)isr6, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT);   // #UD
    idt_set_gate(8, (uint32_t)isr8, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT);   // #DF
    idt_set_gate(13, (uint32_t)isr13, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT); // #GP
                                                                                      // Add other exception handlers (ISRs 1-31) if needed

    // --- Set up IRQ Gates (Hardware Interrupts) ---
    serial_printf("Setting IRQ gates...\n");
    idt_set_gate(32, (uint32_t)irq0, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT); // IRQ 0 (Timer) -> ISR 32
    idt_set_gate(33, (uint32_t)irq1, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT); // IRQ 1 (Keyboard) -> ISR 33
    // Add other IRQ handlers (ISRs 34-47) if needed

    // --- Mask all PIC IRQs initially ---
    // (Common practice: unmask only the ones you are ready to handle)
    serial_printf("Masking all PIC IRQs initially...\n");
    outb(PIC1_DATA, 0xFF); // Mask all IRQs on Master PIC
    outb(PIC2_DATA, 0xFF); // Mask all IRQs on Slave PIC

    // --- Load the IDT register ---
    serial_printf("Loading IDT register (lidt)...\n");
    idt_load(&idt_p);

    // --- Enable Interrupts ---
    // IMPORTANT: Do this *after* everything is set up.
    serial_printf("Enabling interrupts (sti)...\n");
    asm volatile("sti");

    // --- Unmask specific IRQs you want to handle ---
    // DEBUG: Keep Timer (IRQ 0) MASKED for now
    // pic_unmask_irq(0); // Enable Timer IRQ
    serial_printf("Unmasking Keyboard (IRQ 1)...\n");
    pic_unmask_irq(1); // Enable Keyboard IRQ (assuming handler is ready)
}

// --- C-Level Interrupt Handlers (called from assembly stubs) ---

// Generic ISR Handler (for CPU exceptions)
// 'regs' points to the register state pushed by the stub
// void isr_handler(registers_t *regs) // Accept a POINTER
// {
//     serial_printf("!!! CPU Exception %d ", regs->int_no); // Use -> to access members via pointer
//     if (regs->int_no == 8 || regs->int_no == 13)
//     {
//         serial_printf("(Error Code: 0x%x) ", regs->err_code); // Use ->
//     }
//     serial_printf("at EIP=0x%x !!!\n", regs->eip); // Use ->
//     serial_printf("Halting system.\n");

//     asm volatile("cli; hlt");
//     while (1)
//         ;
// }

// Generic IRQ Handler (for hardware interrupts)
// 'regs' points to the register state pushed by the stub
// void irq_handler(registers_t *regs) // Accept a POINTER
// {
//     // Send End-of-Interrupt (EOI) signal(s) *before* handling
//     // Use -> to access members via pointer
//     pic_send_eoi(regs->int_no - 32);

//     // Handle the specific hardware interrupt based on its ISR number
//     if (regs->int_no == 33)
//     {
//         keyboard_handler();
//     }
//     // else if (regs->int_no == 32) {
//     //    timer_handler();
//     // }
// }