#ifndef STRING_H
#define STRING_H

// Define size_t
typedef unsigned int size_t;

// Define NULL if not defined
#ifndef NULL
#define NULL ((void *)0)
#endif

// String length
static inline size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}

// String copy
static inline char *strcpy(char *dest, const char *src)
{
    size_t i = 0;
    while (src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

// String concatenation
static inline char *strcat(char *dest, const char *src)
{
    char *ptr = dest + strlen(dest);
    while (*src != '\0')
    {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

// String comparison
static inline int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Compare first n characters
static inline int strncmp(const char *s1, const char *s2, size_t n)
{
    while (n && *s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
        n--;
    }
    if (n == 0)
        return 0;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Find first occurrence of character in string
static inline char *strchr(const char *str, int ch)
{
    while (*str != '\0')
    {
        if (*str == ch)
        {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

// Find substring in string
static inline char *strstr(const char *haystack, const char *needle)
{
    if (*needle == '\0')
    {
        return (char *)haystack;
    }

    while (*haystack != '\0')
    {
        const char *h = haystack;
        const char *n = needle;

        while (*h == *n && *n != '\0')
        {
            h++;
            n++;
        }

        if (*n == '\0')
        {
            return (char *)haystack;
        }

        haystack++;
    }

    return NULL;
}

// Memory copy
static inline void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;

    while (n--)
    {
        *d++ = *s++;
    }

    return dest;
}

// Memory set
static inline void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;

    while (n--)
    {
        *p++ = (unsigned char)c;
    }

    return s;
}

// Split string by delimiter - this is a simplified version
static inline char *strtok(char *str, const char *delim)
{
    static char *last = NULL;

    if (str != NULL)
    {
        last = str;
    }
    else if (last == NULL)
    {
        return NULL;
    }

    // Skip leading delimiters
    while (*last != '\0')
    {
        const char *d = delim;
        int is_delim = 0;

        while (*d != '\0')
        {
            if (*last == *d)
            {
                is_delim = 1;
                break;
            }
            d++;
        }

        if (!is_delim)
        {
            break;
        }

        last++;
    }

    if (*last == '\0')
    {
        return NULL;
    }

    char *ret = last;

    // Find end of token
    while (*last != '\0')
    {
        const char *d = delim;
        int is_delim = 0;

        while (*d != '\0')
        {
            if (*last == *d)
            {
                is_delim = 1;
                break;
            }
            d++;
        }

        if (is_delim)
        {
            *last = '\0';
            last++;
            break;
        }

        last++;
    }

    return ret;
}

#endif /* STRING_H */