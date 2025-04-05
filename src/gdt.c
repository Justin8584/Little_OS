// gdt.c - GDT initialization
#include "gdt.h"
#include "common.h"

// Define the GDT array (3 entries: Null, Code, Data)
#define GDT_ENTRIES 3
struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gdt_p;

// External ASM function to load GDT register (lgdt) and update segment registers
// We'll create this in gdt_asm.s
extern void gdt_flush(struct gdt_ptr *gdt_p_addr);

// Function to set a GDT entry
// num: Entry number (0, 1, 2...)
// base: Linear base address of segment
// limit: Max addressable unit (limit, up to 2^20 bytes or pages)
// access: Access flags byte
// granularity: Granularity byte
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F); // Upper 4 bits of limit

    gdt[num].granularity |= (granularity & 0xF0); // Add G, D/B, L, AVL flags
    gdt[num].access = access;                     // Set access flags (P, DPL, S, Type)
}

// Initialize GDT
void gdt_init()
{
    // Set up GDT pointer
    gdt_p.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gdt_p.base = (uint32_t)&gdt;

    // GDT Entry 0: Null Segment (required)
    gdt_set_gate(0, 0, 0, 0, 0);

    // GDT Entry 1: Kernel Code Segment
    // Base=0, Limit=4GB, Access=P(1) DPL(0) S(1) Type(Execute/Read=A), Granularity=G(1) D/B(1=32bit)
    // Access byte 0x9A = 1001 1010b (Present=1, DPL=00, S=1, Type=1010 E=1,R=1,A=0)
    // Granularity byte 0xCF = 1100 1111b (G=1, D/B=1, L=0, AVL=0 | Limit[19:16]=1111)
    gdt_set_gate(1, 0x00000000, 0xFFFFFFFF, 0x9A, 0xCF);

    // GDT Entry 2: Kernel Data Segment
    // Base=0, Limit=4GB, Access=P(1) DPL(0) S(1) Type(Read/Write=2), Granularity=G(1) D/B(1=32bit)
    // Access byte 0x92 = 1001 0010b (Present=1, DPL=00, S=1, Type=0010 W=1,A=0)
    // Granularity byte 0xCF = 1100 1111b (G=1, D/B=1, L=0, AVL=0 | Limit[19:16]=1111)
    gdt_set_gate(2, 0x00000000, 0xFFFFFFFF, 0x92, 0xCF);

    // Load the GDT using the assembly function
    gdt_flush(&gdt_p);
}