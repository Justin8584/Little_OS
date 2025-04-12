// common.h - Basic types and structures for freestanding environment
#ifndef COMMON_H
#define COMMON_H

typedef unsigned int size_t;
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#define NULL ((void *)0)

typedef struct
{
    uint32_t ds;
    uint32_t edi, esi, ebp, esp_useless, ebx, edx, ecx, eax;
    uint32_t int_no;
    uint32_t err_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp;
    uint32_t ss;
} registers_t;

#endif