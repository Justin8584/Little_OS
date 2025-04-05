// idt.c - IDT and PIC initialization (sti UNCOMMENTED)
#include "idt.h"
#include "io.h"     // For outb (needed for PIC)
#include "string.h" // For memset
#include "common.h" // For types
#include "fb.h"     // For printing messages (optional)

#define IDT_ENTRIES 256
#define KERNEL_CODE_SEGMENT 0x08      // Your kernel code segment selector from GDT
#define IDT_INTERRUPT_GATE_32BIT 0x8E // P=1, DPL=0, S=0, Type=0xE (32-bit interrupt gate)

// --- Globals ---
// The actual Interrupt Descriptor Table (array of entries)
struct idt_entry idt[IDT_ENTRIES];
// The pointer structure used by the lidt instruction
struct idt_ptr idt_p;

// --- External ASM function ---
// Defined in idt_asm.s, loads the IDTR register
extern void idt_load(struct idt_ptr *idt_p_addr);

// --- Function Implementations ---

// Function to set an IDT entry (gate)
void idt_set_gate(uint8_t num, uint32_t base, uint16_t segment_selector, uint8_t flags)
{
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].segment_selector = segment_selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

// --- PIC Remapping (Essential!) ---
#define PIC1 0x20
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2 0xA0
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20
#define ICW1_ICW4 0x01
#define ICW1_SINGLE 0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL 0x08
#define ICW1_INIT 0x10
#define ICW4_8086 0x01
#define ICW4_AUTO 0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM 0x10

void pic_remap(int offset1, int offset2)
{
    unsigned char mask1, mask2;
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    outb(PIC1_DATA, mask1); // Restore original masks
    outb(PIC2_DATA, mask2);
}

// --- IDT Initialization ---
void idt_init()
{
    idt_p.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idt_p.base = (uint32_t)&idt;
    memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTRIES);
    pic_remap(0x20, 0x28); // Remap IRQs to 0x20-0x2F (32-47)

    // Set up ISR Gates (ensure stubs exist in idt_asm.s)
    idt_set_gate(0, (uint32_t)isr0, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT);
    // Add others if needed

    // Set up IRQ Gates (ensure stubs exist in idt_asm.s)
    idt_set_gate(32, (uint32_t)irq0, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT); // Timer
    idt_set_gate(33, (uint32_t)irq1, KERNEL_CODE_SEGMENT, IDT_INTERRUPT_GATE_32BIT); // Keyboard
    // Add others if needed

    // Load the IDT register
    idt_load(&idt_p);

    // Enable interrupts processor-wide using the 'sti' instruction
    asm volatile("sti"); // <<< Ensure this is UNCOMMENTED
}