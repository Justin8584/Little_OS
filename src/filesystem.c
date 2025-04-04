// src/filesystem.c
#include "filesystem.h"
#include "uart.h"
#include "string.h"

static File files[MAX_FILES];
static int file_count = 0;

// Initialize the filesystem
void fs_init(void)
{
    file_count = 0;
    for (int i = 0; i < MAX_FILES; i++)
    {
        files[i].name[0] = '\0';
        files[i].size = 0;
    }
}

// Find a file by name, return index or -1 if not found
static int find_file(const char *filename)
{
    for (int i = 0; i < file_count; i++)
    {
        if (strcmp(files[i].name, filename) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Create a new file
int fs_create(const char *filename)
{
    if (file_count >= MAX_FILES)
    {
        return -1; // Filesystem full
    }

    if (find_file(filename) != -1)
    {
        return -2; // File already exists
    }

    // Create the file
    strcpy(files[file_count].name, filename);
    files[file_count].size = 0;
    file_count++;

    return 0; // Success
}

// List all files in the filesystem
int fs_list(void)
{
    if (file_count == 0)
    {
        uart_puts("Filesystem is empty\n");
        return 0;
    }

    uart_puts("Files in the filesystem:\n");
    for (int i = 0; i < file_count; i++)
    {
        uart_puts(files[i].name);
        uart_puts(" (");
        // Convert size to string and print
        char size_str[16];
        int size = files[i].size;
        int j = 0;

        if (size == 0)
        {
            size_str[j++] = '0';
        }
        else
        {
            while (size > 0)
            {
                size_str[j++] = '0' + (size % 10);
                size /= 10;
            }
        }

        size_str[j] = '\0';

        // Reverse the string
        for (int k = 0; k < j / 2; k++)
        {
            char tmp = size_str[k];
            size_str[k] = size_str[j - k - 1];
            size_str[j - k - 1] = tmp;
        }

        uart_puts(size_str);
        uart_puts(" bytes)\n");
    }

    return file_count;
}

// Rename a file
int fs_rename(const char *oldname, const char *newname)
{
    int idx = find_file(oldname);
    if (idx == -1)
    {
        return -1; // File not found
    }

    if (find_file(newname) != -1)
    {
        return -2; // New name already exists
    }

    strcpy(files[idx].name, newname);
    return 0; // Success
}

// Delete a file
int fs_delete(const char *filename)
{
    int idx = find_file(filename);
    if (idx == -1)
    {
        return -1; // File not found
    }

    // Remove the file by shifting all files after it
    for (int i = idx; i < file_count - 1; i++)
    {
        strcpy(files[i].name, files[i + 1].name);
        strcpy(files[i].data, files[i + 1].data);
        files[i].size = files[i + 1].size;
    }

    file_count--;
    return 0; // Success
}

// Read from a file
int fs_read(const char *filename, char *buffer, unsigned int size)
{
    int idx = find_file(filename);
    if (idx == -1)
    {
        return -1; // File not found
    }

    unsigned int bytes_to_read = size;
    if (bytes_to_read > files[idx].size)
    {
        bytes_to_read = files[idx].size;
    }

    for (unsigned int i = 0; i < bytes_to_read; i++)
    {
        buffer[i] = files[idx].data[i];
    }

    return bytes_to_read;
}

// Write to a file
int fs_write(const char *filename, const char *data, unsigned int size)
{
    int idx = find_file(filename);
    if (idx == -1)
    {
        return -1; // File not found
    }

    if (size > MAX_FILE_SIZE)
    {
        size = MAX_FILE_SIZE;
    }

    for (unsigned int i = 0; i < size; i++)
    {
        files[idx].data[i] = data[i];
    }
    files[idx].size = size;

    return size;
}