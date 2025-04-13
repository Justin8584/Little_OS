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

// Copy 'n' bytes from 'src' to 'dest'
void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}

// Compare 'n' bytes of 's1' and 's2'
int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (size_t i = 0; i < n; i++)
    {
        if (p1[i] != p2[i])
        {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

// Get the length of string 's'
size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
    {
        len++;
    }
    return len;
}

// Find the first occurrence of character 'c' in string 's'
char *strchr(const char *s, int c)
{
    char ch = (char)c;

    while (*s != '\0')
    {
        if (*s == ch)
        {
            return (char *)s;
        }
        s++;
    }

    if (ch == '\0')
    {
        return (char *)s;
    }

    return NULL;
}

// Find the first occurrence of substring 'needle' in string 'haystack'
char *strstr(const char *haystack, const char *needle)
{
    size_t needle_len = strlen(needle);

    if (needle_len == 0)
    {
        return (char *)haystack;
    }

    while (*haystack)
    {
        if (*haystack == *needle && strncmp(haystack, needle, needle_len) == 0)
        {
            return (char *)haystack;
        }
        haystack++;
    }

    return NULL;
}

// Compare two strings
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Compare at most 'n' characters of two strings
int strncmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
    {
        return 0;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Copy string 'src' to 'dest'
char *strcpy(char *dest, const char *src)
{
    char *original_dest = dest;
    while ((*dest++ = *src++))
        ;
    return original_dest;
}

// Copy at most 'n' characters from 'src' to 'dest'
char *strncpy(char *dest, const char *src, size_t n)
{
    char *original_dest = dest;
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
    {
        dest[i] = '\0';
    }
    return original_dest;
}

// Find the last occurrence of character 'c' in string 's'
const char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    char ch = (char)c;

    while (*s)
    {
        if (*s == ch)
        {
            last = s;
        }
        s++;
    }
    return last;
}
