// fb.c - Framebuffer driver implementation
#include "fb.h"
#include "io.h"
#include "string.h" // For memset in fb_clear scrolling logic if needed

// Framebuffer memory address
static char *fb = (char *)0x000B8000;

// Current cursor position (simple state)
static unsigned short cursor_row = 0;
static unsigned short cursor_col = 0;

// Framebuffer I/O ports
#define FB_COMMAND_PORT 0x3D4
#define FB_DATA_PORT 0x3D5

// Framebuffer commands
#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND 15

// Internal function to move cursor based on linear position
void fb_move_cursor_internal(unsigned short pos)
{
    outb(FB_COMMAND_PORT, FB_HIGH_BYTE_COMMAND);
    outb(FB_DATA_PORT, ((pos >> 8) & 0x00FF));
    outb(FB_COMMAND_PORT, FB_LOW_BYTE_COMMAND);
    outb(FB_DATA_PORT, pos & 0x00FF);
}

// Move the cursor to a specific row and column
void fb_move_cursor(unsigned short row, unsigned short col)
{
    if (row >= FB_ROWS || col >= FB_COLS)
    {
        // Keep cursor within bounds
        return;
    }
    unsigned short pos = row * FB_COLS + col;
    fb_move_cursor_internal(pos);
    cursor_row = row;
    cursor_col = col;
}

// --- Basic Scrolling ---
// Shifts all lines up by one, clears the last line
void fb_scroll()
{
    unsigned int i;
    // Move rows 1 to FB_ROWS-1 up one row
    for (i = 0; i < (FB_ROWS - 1) * FB_COLS * 2; i++)
    {
        fb[i] = fb[i + FB_COLS * 2];
    }
    // Clear the last row
    for (i = (FB_ROWS - 1) * FB_COLS; i < FB_ROWS * FB_COLS; i++)
    {
        fb_write_cell(i, ' ', FB_WHITE, FB_BLACK);
    }
    // Set cursor to the beginning of the last line
    cursor_row = FB_ROWS - 1;
    cursor_col = 0;
}

// Write a cell (char + colors) at a linear position i
void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg)
{
    if (i >= FB_ROWS * FB_COLS)
        return;                  // Bounds check
    unsigned int fb_idx = i * 2; // Each cell is 2 bytes
    fb[fb_idx] = c;
    fb[fb_idx + 1] = ((bg & 0x0F) << 4) | (fg & 0x0F); // Corrected order: BG then FG
}

// Write a cell (char + colors) at the current cursor position and advance
void fb_write_cell_at_cursor(char c, unsigned char fg, unsigned char bg)
{
    // Handle newline separately
    if (c == '\n')
    {
        cursor_col = 0;
        cursor_row++;
    }
    else
    {
        unsigned short pos = cursor_row * FB_COLS + cursor_col;
        fb_write_cell(pos, c, fg, bg);
        // Advance cursor
        cursor_col++;
    }

    // Wrap cursor to next line if needed
    if (cursor_col >= FB_COLS)
    {
        cursor_col = 0;
        cursor_row++;
    }

    // Scroll if cursor goes past the last row
    if (cursor_row >= FB_ROWS)
    {
        fb_scroll();
    }

    // Update hardware cursor position
    fb_move_cursor(cursor_row, cursor_col);
}

// Write a null-terminated string
void fb_write_string(const char *str, unsigned char fg, unsigned char bg)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        fb_write_cell_at_cursor(str[i], fg, bg);
    }
}

// Clear the screen
void fb_clear()
{
    for (int r = 0; r < FB_ROWS; r++)
    {
        for (int c = 0; c < FB_COLS; c++)
        {
            fb_write_cell(r * FB_COLS + c, ' ', FB_WHITE, FB_BLACK);
        }
    }
    fb_move_cursor(0, 0); // Reset cursor to top-left
}

// Get current cursor position
unsigned short fb_get_cursor_row()
{
    return cursor_row;
}

unsigned short fb_get_cursor_col()
{
    return cursor_col;
}