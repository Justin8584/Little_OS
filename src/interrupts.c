// interrupts.c - C level interrupt handlers (Ready for IRQ test)

#include "common.h" // For registers_t, uintN_t, size_t
#include "fb.h"     // For printing, screen manipulation
#include "io.h"     // For inb/outb (keyboard, PIC EOI)
#include "shell.h"  // For shell functions (command execution, buffer management)
#include "interrupts.h"

// Define constants BEFORE use
#define ESC 0x1B // ASCII value for the Escape key

// Access the global command buffer and index (defined in shell.c)
extern char cmd_buffer[];
extern int cmd_buffer_idx;

// --- Keyboard Handling ---
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64 // Not used here, but good to know

// Simple keyboard scan code to ASCII mapping (US QWERTY layout) - Expand as needed
// Index corresponds to scancode. 0 means unhandled/non-printable.
const unsigned char scancode_to_ascii[] = {
    0, ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',           // 0x00-0x0E
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',             // 0x0F-0x1C (Enter)
    0, /*LCTRL*/ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',           // 0x1D-0x29
    0, /*LSHIFT*/ '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, /*RSHIFT*/ // 0x2A-0x36
    0, /*KP **/ 0, /*LALT*/ ' ', 0, /*CAPS*/                                            // 0x37-0x3A
    // F1-F10, NumLock, ScrollLock, Keypad 7-9, -, Keypad 4-6, +, Keypad 1-3, 0, . ...
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* F1-F10 */
    0,                            /* NUMLOCK */
    0,                            /* SCROLLLOCK */
    0,                            /* HOME */
    0,                            /* UP */
    0,                            /* PGUP */
    '-',
    0,   /* LEFT */
    '5', // Keypad 5 assumes NumLock is on/handled
    0,   /* RIGHT */
    '+',
    0, /* END */
    0, /* DOWN */
    0, /* PGDN */
    0, /* INS */
    0, /* DEL */
};

// --- Keyboard Handler function (Restored, but NOT called from irq_handler yet) ---
void keyboard_handler()
{
    unsigned char scancode = inb(KEYBOARD_DATA_PORT);

    // Basic handling: ignore key release events (top bit set) for now
    if (scancode & 0x80)
    {
        // Key release - could handle shift/ctrl release here if needed
        return;
    }

    // Handle key press - Check if scancode is within our defined mapping range
    if (scancode < sizeof(scancode_to_ascii) && scancode_to_ascii[scancode] != 0)
    {
        char ascii = scancode_to_ascii[scancode];

        if (ascii == '\n')
        {
            fb_write_cell_at_cursor('\n', FB_WHITE, FB_BLACK); // Echo newline
            cmd_buffer[cmd_buffer_idx] = '\0';                 // Null-terminate (in shell's buffer)
            if (cmd_buffer_idx > 0)
            {                                  // Only run if command is not empty
                run_shell_command(cmd_buffer); // Call shell function to execute
            }
            clear_cmd_buffer();                       // Call shell function to reset buffer
            fb_write_string("> ", FB_CYAN, FB_BLACK); // Show prompt again
        }
        else if (ascii == '\b')
        { // Handle backspace
            if (cmd_buffer_idx > 0)
            {
                cmd_buffer_idx--;
                // Erase character on screen
                unsigned short current_col = fb_get_cursor_col();
                unsigned short current_row = fb_get_cursor_row();
                if (current_col == 0)
                {
                    if (current_row > 0)
                    {
                        current_row--;
                        current_col = FB_COLS - 1;
                    } // else, already at 0,0, can't backspace further visually
                }
                else
                {
                    current_col--;
                }
                fb_move_cursor(current_row, current_col);
                fb_write_cell_at_cursor(' ', FB_WHITE, FB_BLACK); // Write space (advances cursor)
                fb_move_cursor(current_row, current_col);         // Move back again over the space
            }
        }
        else
        {
            // Add character to buffer if space available
            if (cmd_buffer_idx < CMD_BUFFER_SIZE - 1)
            {
                cmd_buffer[cmd_buffer_idx++] = ascii;
                fb_write_cell_at_cursor(ascii, FB_WHITE, FB_BLACK); // Echo character to screen
            }
        }
    }
}

// --- Interrupt Handlers ---

#define PIC1_COMMAND_PORT 0x20 // Master PIC command port
#define PIC2_COMMAND_PORT 0xA0 // Slave PIC command port
#define PIC_EOI 0x20           // End-of-interrupt command code

// Generic ISR Handler (for CPU exceptions - Restored)
// Called from isr_common_stub in idt_asm.s
// 'regs' points to the register state on the stack
void isr_handler(registers_t *regs)
{
    fb_write_string("CPU Exception: ", FB_RED, FB_BLACK);
    // Ensure fb_write_dec is declared (in shell.h/fb.h) and defined (in shell.c/fb.c)
    fb_write_dec(regs->int_no);
    fb_write_string(" Error Code: ", FB_RED, FB_BLACK);
    fb_write_dec(regs->err_code);
    fb_write_string("\nHalting system.\n", FB_RED, FB_BLACK);
    // Halt the system completely on unexpected exceptions
    asm volatile("cli; hlt");
    while (1)
        ; // Should not be reached
}

// Generic IRQ Handler (for hardware interrupts - Restored, but keyboard call commented out)
// Called from irq_common_stub in idt_asm.s
// 'regs' points to the register state on the stack
void irq_handler(registers_t *regs)
{
    // Send End-of-Interrupt (EOI) signal(s) to the PIC(s)
    if (regs->int_no >= 40)
    { // IRQ 8-15
        outb(PIC2_COMMAND_PORT, PIC_EOI);
    }
    outb(PIC1_COMMAND_PORT, PIC_EOI); // Always send to Master

    // Handle the specific hardware interrupt based on its number
    if (regs->int_no == 33)
    {                       // IRQ 1 (Keyboard)
        keyboard_handler(); // This function uses inb()
    }
    else if (regs->int_no == 32)
    { // IRQ 0 (Timer)
      // Timer handler code would go here
    }
    // ...
}