// kmain.c - Restore full operation for IRQ test

#include "common.h"
#include "fb.h"
#include "gdt.h"
#include "idt.h"
#include "shell.h"
// #include "multiboot.h" // Needed only if checking magic number

unsigned long global_mb_info_addr = 0;

void kmain(unsigned long multiboot_magic, unsigned long multiboot_info_addr)
{
    global_mb_info_addr = multiboot_info_addr;
    (void)multiboot_magic; // Mark as unused for now

    fb_clear();
    fb_write_string("Simple OS Booting...\n", FB_GREEN, FB_BLACK);

    gdt_init(); // Initialize GDT first
    fb_write_string("GDT Initialized.\n", FB_WHITE, FB_BLACK);

    idt_init(); // Initialize IDT and enable interrupts (sti)
    fb_write_string("IDT Initialized.\n", FB_LIGHT_BLUE, FB_BLACK);

    shell_init(); // Initialize shell state
    fb_write_string("Starting Shell...\n", FB_LIGHT_BROWN, FB_BLACK);
    shell_run(); // Prints ">" and enters hlt loop (waiting for IRQs)

    // --- Should not be reached in this design ---
    fb_write_string("Kernel: shell_run returned unexpectedly. Halting.\n", FB_RED, FB_BLACK);
    asm volatile("cli; hlt");
    while (1)
        ;
}