// shell.h - Declarations for shell functions
#ifndef SHELL_H
#define SHELL_H

#include "common.h" // For size_t, uintN_t

#define CMD_BUFFER_SIZE 256 // Define the command buffer size

// Initialize shell state (e.g., clear buffer)
void shell_init();

// Start the shell (e.g., show prompt, enter loop or wait state)
void shell_run();

// Executes a command string (called by keyboard handler)
void run_shell_command(const char *command);

// Clears the internal command buffer (called by keyboard handler)
void clear_cmd_buffer();

// Declare utility functions defined in shell.c if needed elsewhere
void fb_write_dec(unsigned int n); // Function to print decimal numbers

#endif // SHELL_H