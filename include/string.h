// string.h - Basic string/memory function declarations for freestanding
#ifndef STRING_H
#define STRING_H

#include "common.h" // For size_t

// Set 'n' bytes of memory starting at 's' to value 'c'
void *memset(void *s, int c, size_t n);

// Compare two null-terminated strings (already implemented in shell.c, could move here)
// int strcmp(const char* s1, const char* s2);

// Compare first 'n' bytes of two strings (already implemented in shell.c, could move here)
// int strncmp(const char *s1, const char *s2, size_t n);

#endif // STRING_H