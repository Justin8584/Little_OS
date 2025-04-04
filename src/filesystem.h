// src/filesystem.h
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#define MAX_FILES 16
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 1024

typedef struct
{
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    unsigned int size;
} File;

void fs_init(void);
int fs_create(const char *filename);
int fs_list(void);
int fs_rename(const char *oldname, const char *newname);
int fs_delete(const char *filename);
int fs_read(const char *filename, char *buffer, unsigned int size);
int fs_write(const char *filename, const char *data, unsigned int size);

#endif