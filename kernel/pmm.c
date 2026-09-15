#include "pmm.h"
typedef struct
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) e820_entry_t;
#define E820_COUNT_ADDRESS 0x9000
#define E820_MAP_ADDRESS 0x9004
#define MAX_FRAMES 32768
static uint8_t frame_bitmap[MAX_FRAMES / 8];
static uint32_t total_frames = 0;
static uint32_t free_frames = 0;

void pmm_init(void)
{
    uint32_t i;
    uint32_t j;
    uint32_t count;
    for(i=0; i< MAX_FRAMES / 8 ; i++)
    {
        frame_bitmap[i] =0xFF;
    }
    count = *(uint32_t *)E820_COUNT_ADDRESS;

    total_frames = 0;
    free_frames = 0;

    for( i =0; i < count; i++)
    {
        e820_entry_t *entry = (e820_entry_t *)(E820_MAP_ADDRESS + i *sizeof(e820_entry_t));

        if(entry->type != 1)
        {
            continue;
        }
        uint64_t start = entry->base;
        uint64_t end = entry->base + entry->length;
        for(j = (uint32_t)(start/4096);
            j < (uint32_t)(end/4096) && j < MAX_FRAMES; j++)
        {
            frame_bitmap[j/8] &= ~(1 << (j%8));
            free_frames++;
        }
    }

    for(i=0; i< MAX_FRAMES; i++)
    {
        if((frame_bitmap[i/8] & (1 << (i%8))) == 0)
        {
              total_frames++;
        }
    }
}

uint32_t pmm_alloc_frame(void)
{
    uint32_t i;
    uint32_t bit;

    for(i=0; i < MAX_FRAMES ; i++)
    {
        bit = i%8;
        if((frame_bitmap[i/8] & (1 << bit)) == 0)
        {
            frame_bitmap[i/8] |= (1 << bit);
            free_frames --;

            return i*4096;
        }
    }

    return 0xFFFFFFFF;
}

void pmm_free_frame(uint32_t frame)
{
    uint32_t bit;
    frame = frame / 4096;

    if(frame >= MAX_FRAMES)
    { return;}

    bit = frame % 8;

    if(frame_bitmap[frame / 8] & (1 << bit))
    {
        frame_bitmap[frame / 8] &= ~(1 << bit);
        free_frames++;
    }
}

uint32_t pmm_get_total_frames(void)
{
    return total_frames;
}

uint32_t pmm_get_free_frames(void)
{
    return free_frames;
}