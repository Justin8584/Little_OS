// kmalloc.c - Kernel Memory Allocator implementation
#include "kmalloc.h"
#include "pmm.h"
#include "fb.h"
#include "shell.h"

// Minimum block size (including header)
#define MIN_BLOCK_SIZE sizeof(block_header_t)

// Memory alignment (must be power of 2)
#define ALIGN 8
#define ALIGN_MASK (ALIGN - 1)
#define ALIGN_SIZE(size) (((size) + ALIGN_MASK) & ~ALIGN_MASK)

// Head of the free list
static block_header_t *free_list_head = NULL;

// Debug counter
static uint32_t allocated_bytes = 0;
static uint32_t allocated_blocks = 0;

// Initialize the kernel heap allocator
void kheap_init()
{
    // Allocate initial heap area (e.g., 16KB)
    void *initial_heap = pmm_alloc_frame();
    if (!initial_heap)
    {
        fb_write_string("Failed to initialize heap\n", FB_RED, FB_BLACK);
        return;
    }

    // Setup initial free block
    free_list_head = (block_header_t *)initial_heap;
    free_list_head->size = PAGE_SIZE;
    free_list_head->is_free = 1;
    free_list_head->next = NULL;

    fb_write_string("Kernel heap initialized\n", FB_GREEN, FB_BLACK);
}

// Split a block if it's too large
static void split_block(block_header_t *block, size_t size)
{
    // Only split if the remainder would be large enough for a new block
    if (block->size >= size + MIN_BLOCK_SIZE + ALIGN)
    {
        block_header_t *new_block = (block_header_t *)((uint8_t *)block + size);
        new_block->size = block->size - size;
        new_block->is_free = 1;
        new_block->next = block->next;

        block->size = size;
        block->next = new_block;
    }
}

// Expand the heap by allocating more frames
static void expand_heap()
{
    void *new_frame = pmm_alloc_frame();
    if (!new_frame)
    {
        fb_write_string("Out of memory: Failed to expand heap\n", FB_RED, FB_BLACK);
        return;
    }

    // Create a new free block
    block_header_t *new_block = (block_header_t *)new_frame;
    new_block->size = PAGE_SIZE;
    new_block->is_free = 1;
    new_block->next = NULL;

    // Find the end of the free list and append the new block
    if (!free_list_head)
    {
        free_list_head = new_block;
    }
    else
    {
        block_header_t *current = free_list_head;
        while (current->next)
        {
            current = current->next;
        }
        current->next = new_block;
    }
}

// Try to merge adjacent free blocks
static void merge_free_blocks()
{
    block_header_t *current = free_list_head;

    while (current && current->next)
    {
        if (current->is_free && current->next->is_free &&
            (uint8_t *)current + current->size == (uint8_t *)current->next)
        {
            // Merge with next block
            current->size += current->next->size;
            current->next = current->next->next;
        }
        else
        {
            current = current->next;
        }
    }
}

// Allocate memory
void *kmalloc(size_t size)
{
    if (size == 0)
        return NULL;

    // Align size and add header
    size_t aligned_size = ALIGN_SIZE(size + sizeof(block_header_t));

    // Ensure minimum block size
    if (aligned_size < MIN_BLOCK_SIZE)
    {
        aligned_size = MIN_BLOCK_SIZE;
    }

    // Try to find a suitable free block
    block_header_t *current = free_list_head;
    // Removed the unused 'previous' variable

    while (current)
    {
        if (current->is_free && current->size >= aligned_size)
        {
            // Found a suitable block
            split_block(current, aligned_size);
            current->is_free = 0;

            // Update debug stats
            allocated_bytes += aligned_size;
            allocated_blocks++;

            // Return address after the header
            return (void *)((uint8_t *)current + sizeof(block_header_t));
        }

        current = current->next;
    }

    // No suitable block found, expand the heap
    expand_heap();

    // Try again
    return kmalloc(size);
}

// Free previously allocated memory
void kfree(void *ptr)
{
    if (!ptr)
        return;

    // Get the block header
    block_header_t *block = (block_header_t *)((uint8_t *)ptr - sizeof(block_header_t));

    // Mark as free
    block->is_free = 1;

    // Update debug stats
    allocated_bytes -= block->size;
    allocated_blocks--;

    // Try to merge with adjacent free blocks
    merge_free_blocks();
}

// Print heap statistics
void kheap_stats()
{
    fb_write_string("Heap statistics:\n", FB_GREEN, FB_BLACK);
    fb_write_string("Allocated blocks: ", FB_WHITE, FB_BLACK);
    fb_write_dec(allocated_blocks);
    fb_write_string("\nAllocated bytes: ", FB_WHITE, FB_BLACK);
    fb_write_dec(allocated_bytes);
    fb_write_string("\n", FB_WHITE, FB_BLACK);

    // Count free blocks and total free memory
    uint32_t free_blocks = 0;
    uint32_t free_bytes = 0;
    block_header_t *current = free_list_head;

    while (current)
    {
        if (current->is_free)
        {
            free_blocks++;
            free_bytes += current->size;
        }
        current = current->next;
    }

    fb_write_string("Free blocks: ", FB_WHITE, FB_BLACK);
    fb_write_dec(free_blocks);
    fb_write_string("\nFree bytes: ", FB_WHITE, FB_BLACK);
    fb_write_dec(free_bytes);
    fb_write_string("\n", FB_WHITE, FB_BLACK);
}