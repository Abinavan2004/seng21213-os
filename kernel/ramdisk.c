#include "../include/types.h"
#define RAMDISK_SIZE (1024*1024)

static uint8_t ramdisk[RAMDISK_SIZE];

uint8_t *ramdisk_get(void)
{
    return ramdisk;
}

uint32_t ramdisk_size(void)
{
    return RAMDISK_SIZE;
}