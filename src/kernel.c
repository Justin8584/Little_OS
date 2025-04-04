// src/kernel.c
#include "uart.h"
#include "filesystem.h"
#include "process.h"
#include "string.h"

// Simple utility to convert integer to string
void itoa(int num, char *str)
{
    int i = 0;
    int is_negative = 0;

    // Handle 0 explicitly
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Handle negative numbers
    if (num < 0)
    {
        is_negative = 1;
        num = -num;
    }

    // Convert digits in reverse order
    while (num != 0)
    {
        str[i++] = (num % 10) + '0';
        num = num / 10;
    }

    // Add negative sign if needed
    if (is_negative)
    {
        str[i++] = '-';
    }

    str[i] = '\0';

    // Reverse the string
    int start = 0;
    int end = i - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// Simple shell to demonstrate OS functions
void shell()
{
    char input[100];
    int input_pos = 0;

    uart_puts("\n\n==== MyOS Shell ====\n");
    uart_puts("Available commands: help, createfile, listfiles, renamefile, deletefile, createproc, listproc, killproc\n");

    while (1)
    {
        uart_puts("\n> ");

        // Read input
        input_pos = 0;
        while (1)
        {
            char c = uart_getc();

            // Echo back
            uart_putc(c);

            // Handle backspace
            if (c == 127 || c == 8)
            {
                if (input_pos > 0)
                {
                    input_pos--;
                    // Echo back space, backspace, space to erase character
                    uart_puts("\b \b");
                }
                continue;
            }

            // Handle enter
            if (c == '\r' || c == '\n')
            {
                uart_puts("\n");
                input[input_pos] = '\0';
                break;
            }

            // Store character
            if (input_pos < 99)
            {
                input[input_pos++] = c;
            }
        }

        // Parse and execute command
        if (strcmp(input, "help") == 0)
        {
            uart_puts("Available commands:\n");
            uart_puts("  help - Show this help\n");
            uart_puts("  createfile <name> - Create a new file\n");
            uart_puts("  listfiles - List all files\n");
            uart_puts("  renamefile <old> <new> - Rename a file\n");
            uart_puts("  deletefile <name> - Delete a file\n");
            uart_puts("  createproc <name> - Create a new process\n");
            uart_puts("  listproc - List all processes\n");
            uart_puts("  killproc <pid> - Terminate a process\n");
        }
        else if (strncmp(input, "createfile ", 11) == 0)
        {
            char *filename = input + 11;
            int result = fs_create(filename);
            if (result == 0)
            {
                uart_puts("File created: ");
                uart_puts(filename);
                uart_puts("\n");
            }
            else
            {
                uart_puts("Failed to create file\n");
            }
        }
        else if (strcmp(input, "listfiles") == 0)
        {
            fs_list();
        }
        else if (strncmp(input, "renamefile ", 11) == 0)
        {
            char *rest = input + 11;
            char *space = strchr(rest, ' ');

            if (space)
            {
                *space = '\0';
                char *oldname = rest;
                char *newname = space + 1;

                int result = fs_rename(oldname, newname);
                if (result == 0)
                {
                    uart_puts("File renamed from ");
                    uart_puts(oldname);
                    uart_puts(" to ");
                    uart_puts(newname);
                    uart_puts("\n");
                }
                else
                {
                    uart_puts("Failed to rename file\n");
                }
            }
            else
            {
                uart_puts("Usage: renamefile <old> <new>\n");
            }
        }
        else if (strncmp(input, "deletefile ", 11) == 0)
        {
            char *filename = input + 11;
            int result = fs_delete(filename);
            if (result == 0)
            {
                uart_puts("File deleted: ");
                uart_puts(filename);
                uart_puts("\n");
            }
            else
            {
                uart_puts("Failed to delete file\n");
            }
        }
        else if (strncmp(input, "createproc ", 11) == 0)
        {
            char *procname = input + 11;
            int pid = process_create(procname);
            if (pid > 0)
            {
                uart_puts("Process created: ");
                uart_puts(procname);
                uart_puts(" (PID: ");
                char pid_str[16];
                itoa(pid, pid_str);
                uart_puts(pid_str);
                uart_puts(")\n");
            }
            else
            {
                uart_puts("Failed to create process\n");
            }
        }
        else if (strcmp(input, "listproc") == 0)
        {
            process_list();
        }
        else if (strncmp(input, "killproc ", 9) == 0)
        {
            char *pid_str = input + 9;
            int pid = 0;

            // Parse PID
            for (int i = 0; pid_str[i] != '\0'; i++)
            {
                if (pid_str[i] >= '0' && pid_str[i] <= '9')
                {
                    pid = pid * 10 + (pid_str[i] - '0');
                }
                else
                {
                    pid = -1;
                    break;
                }
            }

            if (pid > 0)
            {
                int result = process_kill(pid);
                if (result == 0)
                {
                    uart_puts("Process terminated: PID ");
                    char buf[16];
                    itoa(pid, buf);
                    uart_puts(buf);
                    uart_puts("\n");
                }
                else
                {
                    uart_puts("Failed to terminate process\n");
                }
            }
            else
            {
                uart_puts("Invalid PID\n");
            }
        }
        else if (input_pos > 0)
        {
            uart_puts("Unknown command: ");
            uart_puts(input);
            uart_puts("\n");
        }
    }
}

// Kernel entry point
void main()
{
    // Initialize hardware
    uart_init();

    // Welcome message
    uart_puts("\n\n===================================\n");
    uart_puts("      MyOS for ARM - v1.0.0      \n");
    uart_puts("===================================\n");

    // Initialize subsystems
    fs_init();
    process_init();

    // Create some initial files and processes for demonstration
    fs_create("readme.txt");
    fs_write("readme.txt", "Welcome to MyOS!\nThis is a simple operating system.", 54);

    fs_create("hello.txt");
    fs_write("hello.txt", "Hello, World!", 13);

    process_create("init");
    process_create("shell");

    // Start shell
    shell();

    // We should never reach here
    while (1)
    {
    }
}