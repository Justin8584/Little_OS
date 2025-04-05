// string.c - Basic string/memory function implementations
#include "string.h"

// Set 'n' bytes of memory starting at 's' to value 'c'
void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    unsigned char value = (unsigned char)c;

    for (size_t i = 0; i < n; i++)
    {
        p[i] = value;
    }
    return s;
}

// Implement strcmp, strncmp here if you want them centrally
// (Currently implemented in shell.c as simple_strcmp/simple_strncmp)