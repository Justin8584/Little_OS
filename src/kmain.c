// kmain.c - Updated with stack check

#include "common.h"
#include "fb.h"
#include "serial.h" // Need serial early!
#include "gdt.h"
#include "idt.h" // Include IDT header
#include "shell.h"
#include "multiboot.h"
#include "pmm.h"
#include "kmalloc.h"
#include "fs.h"
#include "string.h"

// Save the multiboot info address for later use
unsigned long global_mb_info_addr = 0;

void kmain(unsigned long multiboot_magic, unsigned long multiboot_info_addr)
{
    // --- VERY EARLY INIT ---
    serial_configure(); // Defined in serial.c
    serial_printf("\n--- kmain entered ---\n");

    global_mb_info_addr = multiboot_info_addr;
    serial_printf("Multiboot Info Addr: 0x%x\n", multiboot_info_addr);
    serial_printf("Multiboot Magic    : 0x%x\n", multiboot_magic);

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        serial_printf("Error: Invalid multiboot magic number: 0x%x\n", multiboot_magic);
        serial_printf("HALTING.\n");
        asm volatile("cli; hlt");
        while (1)
            ;
    }
    serial_printf("Multiboot magic OK.\n");

    // --- Initialize core components with serial logs ---
    serial_printf("Initializing GDT...\n");
    gdt_init();
    serial_printf("GDT Initialized.\n");

    // --- STACK CHECK ---
    uint32_t current_esp;
    asm volatile("mov %%esp, %0" : "=r"(current_esp));
    serial_printf("ESP before IDT init: 0x%x\n", current_esp);
    // --- END STACK CHECK ---

    serial_printf("Initializing IDT/PIC...\n");
    idt_init(); // This enables interrupts with 'sti' inside it!
    serial_printf("IDT/PIC Initialized (Interrupts Enabled!).\n");
    // Interrupts are now potentially active!

    serial_printf("Initializing PMM...\n");
    pmm_init((multiboot_info_t *)multiboot_info_addr);
    serial_printf("PMM Initialized.\n");

    serial_printf("Initializing Kernel Heap...\n");
    kheap_init();
    serial_printf("Kernel Heap Initialized.\n");

    serial_printf("Initializing Filesystem...\n");
    fs_init();
    serial_printf("Filesystem Initialized.\n");

    serial_printf("Initializing Framebuffer...\n");
    fb_clear();
    fb_write_string("Little OS Booting... (Framebuffer OK)\n", FB_GREEN, FB_BLACK);
    serial_printf("Framebuffer Initialized.\n");

    serial_printf("Creating initial filesystem entries...\n");
    // ... (rest of kmain - filesystem creation, shell init) ...
    // Create some initial files and directories for testing
    fs_mkdir(fs_root, "bin");
    fs_mkdir(fs_root, "home");
    fs_mkdir(fs_root, "etc");

    fs_node_t *home_dir = fs_finddir(fs_root, "home");
    if (home_dir)
    {
        fs_mkdir(home_dir, "user");

        fs_node_t *user_dir = fs_finddir(home_dir, "user");
        if (user_dir)
        {
            // Create a welcome file
            fs_node_t *welcome = fs_create(user_dir, "welcome.txt", FS_FILE);
            if (welcome)
            {
                const char *welcome_text = "Welcome to Little OS!\nThis is a simple in-memory filesystem.";
                fs_write(welcome, 0, strlen(welcome_text), (uint8_t *)welcome_text);
            }
            else
            {
                serial_printf("Failed to create welcome.txt\n");
            }
        }
        else
        {
            serial_printf("Failed to find /home/user\n");
        }
    }
    else
    {
        serial_printf("Failed to find /home\n");
    }
    serial_printf("Initial filesystem entries created.\n");

    // Initialize shell and start running
    serial_printf("Initializing Shell...\n");
    shell_init();
    serial_printf("Shell Initialized.\n");
    fb_write_string("Starting Shell...\n", FB_LIGHT_BROWN, FB_BLACK);
    shell_run(); // Contains the hlt loop

    // Should not be reached
    serial_printf("Kernel: shell_run returned unexpectedly. Halting.\n");
    fb_write_string("Kernel: shell_run returned unexpectedly. Halting.\n", FB_RED, FB_BLACK);
    asm volatile("cli; hlt");
    while (1)
        ;
}