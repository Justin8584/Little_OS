// pmm.c - Physical Memory Manager implementation
#include "pmm.h"
#include "fb.h"

// Bitmap to track used/free frames (1 = used, 0 = free)
static uint32_t *frame_bitmap = NULL;
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;
static uint32_t bitmap_size = 0;
static uint32_t memory_size = 0;

// Initialize frame bitmap at a fixed address initially
#define BITMAP_LOCATION 0x100000 // 1MB mark, adjust as needed

// Initialize the physical memory manager
void pmm_init(multiboot_info_t *mboot_info)
{
    if (!(mboot_info->flags & MULTIBOOT_INFO_MEM_MAP))
    {
        fb_write_string("Error: Memory map not available from bootloader\n", FB_RED, FB_BLACK);
        return;
    }

    // Calculate total memory size from memory map
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mboot_info->mmap_addr;
    uint32_t highest_addr = 0;

    while ((unsigned long)mmap < mboot_info->mmap_addr + mboot_info->mmap_length)
    {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            uint32_t end_addr = (uint32_t)((mmap->addr + mmap->len) & 0xFFFFFFFF);
            if (end_addr > highest_addr)
            {
                highest_addr = end_addr;
            }
        }
        // Move to next entry
        mmap = (multiboot_memory_map_t *)((unsigned long)mmap + mmap->size + sizeof(mmap->size));
    }

    // Calculate total frames and bitmap size
    memory_size = highest_addr;
    total_frames = memory_size / PAGE_SIZE;
    bitmap_size = total_frames / 32;
    if (total_frames % 32)
        bitmap_size++; // Round up if needed

    // Place bitmap at fixed location initially
    frame_bitmap = (uint32_t *)BITMAP_LOCATION;

    // Mark all frames as free initially
    for (uint32_t i = 0; i < bitmap_size; i++)
    {
        frame_bitmap[i] = 0;
    }

    // Mark reserved memory regions as used
    mmap = (multiboot_memory_map_t *)mboot_info->mmap_addr;
    while ((unsigned long)mmap < mboot_info->mmap_addr + mboot_info->mmap_length)
    {
        if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE)
        {
            // Mark this region as used
            uint32_t start_frame = (uint32_t)(mmap->addr / PAGE_SIZE);
            uint32_t end_frame = (uint32_t)((mmap->addr + mmap->len) / PAGE_SIZE);

            for (uint32_t i = start_frame; i < end_frame; i++)
            {
                SET_FRAME(frame_bitmap, i);
                used_frames++;
            }
        }
        // Move to next entry
        mmap = (multiboot_memory_map_t *)((unsigned long)mmap + mmap->size + sizeof(mmap->size));
    }

    // Mark kernel and bitmap memory as used
    // Assuming kernel is located at 1MB and size is known or calculated
    uint32_t kernel_start_frame = 0; // Start of physical memory
    uint32_t kernel_end_frame = (BITMAP_LOCATION + bitmap_size * 4) / PAGE_SIZE + 1;

    for (uint32_t i = kernel_start_frame; i < kernel_end_frame; i++)
    {
        SET_FRAME(frame_bitmap, i);
        used_frames++;
    }

    fb_write_string("PMM initialized: ", FB_GREEN, FB_BLACK);
    fb_write_dec(total_frames);
    fb_write_string(" frames total, ", FB_GREEN, FB_BLACK);
    fb_write_dec(used_frames);
    fb_write_string(" frames used\n", FB_GREEN, FB_BLACK);
}

// Allocate a single frame and return its address
void *pmm_alloc_frame()
{
    if (used_frames >= total_frames)
    {
        return NULL; // Out of memory
    }

    // Find the first free frame
    for (uint32_t i = 0; i < total_frames; i++)
    {
        if (!TEST_FRAME(frame_bitmap, i))
        {
            SET_FRAME(frame_bitmap, i);
            used_frames++;
            return (void *)(i * PAGE_SIZE);
        }
    }

    return NULL; // Should not reach here if used_frames < total_frames
}

// Free a previously allocated frame
void pmm_free_frame(void *frame_addr)
{
    uint32_t frame = (uint32_t)frame_addr / PAGE_SIZE;

    if (frame >= total_frames)
    {
        return; // Invalid frame
    }

    if (TEST_FRAME(frame_bitmap, frame))
    {
        CLEAR_FRAME(frame_bitmap, frame);
        used_frames--;
    }
}

// Get the number of free frames
uint32_t pmm_get_free_frame_count()
{
    return total_frames - used_frames;
}

// Display memory map (for debugging)
void pmm_memory_map()
{
    fb_write_string("Memory Map:\n", FB_GREEN, FB_BLACK);
    fb_write_string("Total memory: ", FB_WHITE, FB_BLACK);
    fb_write_dec(memory_size / 1024);
    fb_write_string(" KB (", FB_WHITE, FB_BLACK);
    fb_write_dec(total_frames);
    fb_write_string(" frames)\n", FB_WHITE, FB_BLACK);
    fb_write_string("Free frames: ", FB_WHITE, FB_BLACK);
    fb_write_dec(total_frames - used_frames);
    fb_write_string("\n", FB_WHITE, FB_BLACK);
}