// fb.h - Framebuffer driver declarations
#ifndef FB_H
#define FB_H

#include "common.h" // For types if needed indirectly
#include "io.h"     // Include for outb (used by cursor functions)

// Framebuffer Colors
#define FB_BLACK 0
#define FB_BLUE 1
#define FB_GREEN 2
#define FB_CYAN 3
#define FB_RED 4
#define FB_MAGENTA 5
#define FB_BROWN 6
#define FB_LIGHT_GREY 7
#define FB_DARK_GREY 8
#define FB_LIGHT_BLUE 9
#define FB_LIGHT_GREEN 10
#define FB_LIGHT_CYAN 11
#define FB_LIGHT_RED 12
#define FB_LIGHT_MAGENTA 13
#define FB_LIGHT_BROWN 14 // Often looks Yellow
#define FB_WHITE 15

// Framebuffer dimensions
#define FB_COLS 80
#define FB_ROWS 25

// Write a character with specified colors to linear position i
void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg);

// Move the cursor to position row, col
void fb_move_cursor(unsigned short row, unsigned short col);

// Write a character with specified colors at the current cursor position
// and advance the cursor (handles scrolling simply).
void fb_write_cell_at_cursor(char c, unsigned char fg, unsigned char bg);

// Write a null-terminated string to the framebuffer starting at the cursor
void fb_write_string(const char *str, unsigned char fg, unsigned char bg);

// Clear the framebuffer
void fb_clear();

// Get current cursor position
unsigned short fb_get_cursor_row();
unsigned short fb_get_cursor_col();

#endif // FB_H