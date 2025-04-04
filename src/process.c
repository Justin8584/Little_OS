// src/process.c
#include "process.h"
#include "uart.h"
#include "string.h"

static Process processes[MAX_PROCESSES];
static unsigned int next_pid = 1;
static int process_count = 0;

// Initialize the process manager
void process_init(void)
{
    process_count = 0;
    next_pid = 1;

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        processes[i].name[0] = '\0';
        processes[i].state = PROCESS_TERMINATED;
        processes[i].pid = 0;
    }
}

// Find a process by PID, return index or -1 if not found
static int find_process(unsigned int pid)
{
    for (int i = 0; i < process_count; i++)
    {
        if (processes[i].pid == pid)
        {
            return i;
        }
    }
    return -1;
}

// Create a new process
int process_create(const char *name)
{
    if (process_count >= MAX_PROCESSES)
    {
        return -1; // Process table full
    }

    // Create the process
    strcpy(processes[process_count].name, name);
    processes[process_count].state = PROCESS_READY;
    processes[process_count].pid = next_pid++;
    process_count++;

    return processes[process_count - 1].pid; // Return the assigned PID
}

// List all processes
int process_list(void)
{
    if (process_count == 0)
    {
        uart_puts("No processes running\n");
        return 0;
    }

    uart_puts("PID\tSTATE\t\tNAME\n");
    uart_puts("--------------------------------\n");

    for (int i = 0; i < process_count; i++)
    {
        // Print PID
        char pid_str[16];
        int pid = processes[i].pid;
        int j = 0;

        if (pid == 0)
        {
            pid_str[j++] = '0';
        }
        else
        {
            while (pid > 0)
            {
                pid_str[j++] = '0' + (pid % 10);
                pid /= 10;
            }
        }

        pid_str[j] = '\0';

        // Reverse the string
        for (int k = 0; k < j / 2; k++)
        {
            char tmp = pid_str[k];
            pid_str[k] = pid_str[j - k - 1];
            pid_str[j - k - 1] = tmp;
        }

        uart_puts(pid_str);
        uart_puts("\t");

        // Print state
        switch (processes[i].state)
        {
        case PROCESS_READY:
            uart_puts("READY\t\t");
            break;
        case PROCESS_RUNNING:
            uart_puts("RUNNING\t\t");
            break;
        case PROCESS_BLOCKED:
            uart_puts("BLOCKED\t\t");
            break;
        case PROCESS_TERMINATED:
            uart_puts("TERMINATED\t");
            break;
        }

        // Print name
        uart_puts(processes[i].name);
        uart_puts("\n");
    }

    return process_count;
}

// Terminate a process
int process_kill(unsigned int pid)
{
    int idx = find_process(pid);
    if (idx == -1)
    {
        return -1; // Process not found
    }

    // Remove the process by shifting all processes after it
    for (int i = idx; i < process_count - 1; i++)
    {
        strcpy(processes[i].name, processes[i + 1].name);
        processes[i].state = processes[i + 1].state;
        processes[i].pid = processes[i + 1].pid;
    }

    process_count--;
    return 0; // Success
}

// Change process state
int process_set_state(unsigned int pid, ProcessState new_state)
{
    int idx = find_process(pid);
    if (idx == -1)
    {
        return -1; // Process not found
    }

    processes[idx].state = new_state;
    return 0; // Success
}