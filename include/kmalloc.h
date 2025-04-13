// kmalloc.h - Kernel Memory Allocator
#ifndef KMALLOC_H
#define KMALLOC_H

#include "common.h"

// Memory block header structure
typedef struct block_header
{
    uint32_t size;             // Size of the block (including header)
    uint8_t is_free;           // 1 if free, 0 if used
    struct block_header *next; // Next block in the list
} block_header_t;

// Function declarations
void kheap_init();
void *kmalloc(size_t size);
void kfree(void *ptr);
void kheap_stats();

#endif // KMALLOC_H