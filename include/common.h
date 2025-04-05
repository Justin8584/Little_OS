// common.h - Basic types and structures for freestanding environment
#ifndef COMMON_H
#define COMMON_H

// Basic size type (usually unsigned int on 32-bit)
typedef unsigned int size_t;

// Fixed-width integer types
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
// Add 64-bit types if needed (long long)
// typedef long long            int64_t;
// typedef unsigned long long   uint64_t;

#define NULL ((void *)0)

// Structure to hold register state pushed by ISR/IRQ stubs
// Matches the layout after pusha and our pushes in idt_asm.s
typedef struct
{
    uint32_t ds;                                             // Pushed last before C call (by our asm stub, data segment)
    uint32_t edi, esi, ebp, esp_useless, ebx, edx, ecx, eax; // Pushed by pusha (esp is ignored)
    uint32_t int_no;                                         // Pushed by our asm stub (interrupt number)
    uint32_t err_code;                                       // Pushed by our asm stub or CPU (error code)
    // --- CPU Pushed (below the part accessed via 'regs' pointer) ---
    uint32_t eip;     // Instruction Pointer
    uint32_t cs;      // Code Segment
    uint32_t eflags;  // Flags register
    uint32_t useresp; // Stack pointer (if privilege change)
    uint32_t ss;      // Stack Segment (if privilege change)
} registers_t;

#endif // COMMON_H