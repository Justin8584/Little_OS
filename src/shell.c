// shell.c - Simple shell implementation

#include "shell.h"
#include "fb.h"
#include "multiboot.h"
#include "common.h"
#include "string.h"

// Access the global MB info address (defined in kmain.c)
extern unsigned long global_mb_info_addr;

// --- Shell State ---
char cmd_buffer[CMD_BUFFER_SIZE];
int cmd_buffer_idx = 0;

// --- Utility Functions ---

// Function to clear the command buffer
void clear_cmd_buffer()
{
    memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
    cmd_buffer_idx = 0;
}

// Basic strcmp (compare two null-terminated strings)
// Returns 0 if equal, <0 if s1 < s2, >0 if s1 > s2
int simple_strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Simplified strncmp (compare up to n characters)
int simple_strncmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2))
    {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0)
    {
        return 0;
    }
    else
    {
        return *(unsigned char *)s1 - *(unsigned char *)s2;
    }
}

// Function to print an unsigned decimal integer to the framebuffer
void fb_write_dec(unsigned int n)
{
    if (n == 0)
    {
        fb_write_cell_at_cursor('0', FB_WHITE, FB_BLACK);
        return;
    }

    char buffer[12]; // Max 10 digits for 32-bit unsigned int + null
    int i = 10;
    buffer[11] = '\0'; // Null terminate

    unsigned int num = n;

    while (num > 0 && i >= 0)
    {
        buffer[i--] = (num % 10) + '0';
        num /= 10;
    }

    // Handle potential case where i didn't reach 0 (shouldn't happen for uint32)
    if (i < 0)
        i = 0;

    fb_write_string(&buffer[i + 1], FB_WHITE, FB_BLACK);
}

// --- Command Execution ---

// Function to execute commands
void run_shell_command(const char *command)
{
    if (simple_strcmp(command, "help") == 0)
    {
        fb_write_string("Available commands:\n", FB_GREEN, FB_BLACK);
        fb_write_string("  help    - Show this help message\n", FB_WHITE, FB_BLACK);
        fb_write_string("  cls     - Clear the screen\n", FB_WHITE, FB_BLACK);
        fb_write_string("  echo    - Print text after command\n", FB_WHITE, FB_BLACK);
        fb_write_string("  meminfo - Show basic memory info (from Multiboot)\n", FB_WHITE, FB_BLACK);
    }
    else if (simple_strcmp(command, "cls") == 0)
    {
        fb_clear();
    }
    else if (simple_strncmp(command, "echo ", 5) == 0)
    {                                                            // Check for "echo " prefix
        const char *text_to_echo = command + 5;                  // Get pointer to text after "echo "
        fb_write_string(text_to_echo, FB_LIGHT_BROWN, FB_BLACK); // Use light brown (yellow)
        fb_write_string("\n", FB_LIGHT_BROWN, FB_BLACK);
    }
    else if (simple_strcmp(command, "meminfo") == 0)
    {
        if (global_mb_info_addr == 0)
        {
            fb_write_string("Error: Multiboot info address not available.\n", FB_RED, FB_BLACK);
            return;
        }
        multiboot_info_t *mb_info = (multiboot_info_t *)global_mb_info_addr;
        fb_write_string("Memory Info (from Multiboot):\n", FB_GREEN, FB_BLACK);

        // Check if memory map is available (preferred)
        if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP)
        {
            multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mb_info->mmap_addr;
            unsigned long total_mem_kb = 0;
            unsigned int entry_count = 0;
            fb_write_string(" Type | Start Addr (low) | Length (KB)\n", FB_CYAN, FB_BLACK);
            fb_write_string("------|------------------|-------------\n", FB_CYAN, FB_BLACK);

            // Iterate through the memory map entries
            while ((unsigned long)mmap < mb_info->mmap_addr + mb_info->mmap_length)
            {
                entry_count++;
                unsigned long len_kb = mmap->len / 1024; // Calculate length in KB

                // Print type
                if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
                {
                    fb_write_string(" Avail| ", FB_WHITE, FB_BLACK);
                    total_mem_kb += len_kb; // Add to total available memory
                }
                else
                {
                    fb_write_string(" Reserv| ", FB_LIGHT_RED, FB_BLACK);
                }

                // Print start address (low 32 bits)
                fb_write_dec((unsigned int)(mmap->addr & 0xFFFFFFFF));
                fb_write_string(" | ", FB_WHITE, FB_BLACK);

                // Print length in KB
                fb_write_dec(len_kb);
                fb_write_string("\n", FB_WHITE, FB_BLACK);

                // Move to the next memory map entry
                mmap = (multiboot_memory_map_t *)((unsigned long)mmap + mmap->size + sizeof(mmap->size));
            }
            fb_write_string("\nTotal Available RAM (from map): ", FB_GREEN, FB_BLACK);
            fb_write_dec(total_mem_kb);
            fb_write_string(" KB (", FB_GREEN, FB_BLACK);
            fb_write_dec(entry_count);
            fb_write_string(" entries)\n", FB_GREEN, FB_BLACK);
        }
        else if (mb_info->flags & MULTIBOOT_INFO_MEMORY)
        {
            // Fallback to simple mem_lower/mem_upper if map not present
            unsigned long total_kb = mb_info->mem_lower + mb_info->mem_upper;
            fb_write_string("Basic Memory Info (lower + upper):\n", FB_WHITE, FB_BLACK);
            fb_write_dec(total_kb);
            fb_write_string(" KB Total\n", FB_WHITE, FB_BLACK);
        }
        else
        {
            fb_write_string("No detailed memory info available from bootloader.\n", FB_RED, FB_BLACK);
        }
    }
    else
    {
        fb_write_string("Unknown command: '", FB_RED, FB_BLACK);
        fb_write_string(command, FB_RED, FB_BLACK);
        fb_write_string("'\n", FB_RED, FB_BLACK);
    }
}

// --- Shell Initialization and Running ---

// Initialize shell state
void shell_init()
{
    clear_cmd_buffer();
}

// Start the shell (display prompt and wait for interrupts)
void shell_run()
{
    fb_write_string("> ", FB_CYAN, FB_BLACK); // Show initial prompt
    // The main processing loop is now driven by keyboard interrupts.
    // The kernel can idle here using 'hlt'.
    while (1)
    {
        asm volatile("hlt"); // Halt CPU until the next interrupt occurs
    }
}