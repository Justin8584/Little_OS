// pmm.h - Physical Memory Manager (Page Frame Allocator)
#ifndef PMM_H
#define PMM_H

#include "common.h"
#include "multiboot.h"

// Constants
#define PAGE_SIZE 4096
#define BITMAP_INDEX(frame) (frame / 32)
#define BITMAP_OFFSET(frame) (frame % 32)
#define SET_FRAME(bitmap, frame) (bitmap[BITMAP_INDEX(frame)] |= (1 << BITMAP_OFFSET(frame)))
#define CLEAR_FRAME(bitmap, frame) (bitmap[BITMAP_INDEX(frame)] &= ~(1 << BITMAP_OFFSET(frame)))
#define TEST_FRAME(bitmap, frame) (bitmap[BITMAP_INDEX(frame)] & (1 << BITMAP_OFFSET(frame)))

// Function declarations
void pmm_init(multiboot_info_t *mboot_info);
void *pmm_alloc_frame();
void pmm_free_frame(void *frame_addr);
uint32_t pmm_get_free_frame_count();
void pmm_memory_map();

#endif // PMM_H