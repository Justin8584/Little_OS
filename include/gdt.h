// gdt.h - GDT structures and functions
#ifndef GDT_H
#define GDT_H

#include "common.h" // For uintN_t types

// GDT Entry structure (64 bits)
struct gdt_entry
{
    uint16_t limit_low;  // Lower 16 bits of limit
    uint16_t base_low;   // Lower 16 bits of base address
    uint8_t base_middle; // Middle 8 bits of base address
    uint8_t access;      // Access flags (P, DPL, S, Type)
    uint8_t granularity; // Granularity (G, D/B, L, AVL) + upper 4 bits of limit
    uint8_t base_high;   // Upper 8 bits of base address
} __attribute__((packed));

// GDT Pointer structure (for lgdt instruction)
struct gdt_ptr
{
    uint16_t limit; // Size of GDT in bytes minus 1
    uint32_t base;  // Linear address of the GDT
} __attribute__((packed));

// Function to initialize GDT and load it
void gdt_init();

#endif // GDT_H