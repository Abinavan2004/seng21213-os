#ifndef FS_H
#define FS_H

#include"../include/types.h"

#define FS_BLOCK_SIZE 4096
#define FS_TOTAL_BLOCKS 256
#define FS_MAX_INODES 64
#define FS_BLOCK_BITMAP_SIZE (FS_TOTAL_BLOCKS / 8)
#define FS_INODE_BITMAP_SIZE (FS_MAX_INODES / 8)
#define FS_MAX_FILENAME 28
#define FS_MAGIC 0x53454E47

typedef struct
{
    uint32_t magic;
    uint32_t block_count;
    uint32_t inode_count;

} superblock_t;

typedef struct __attribute__((packed))
{

    uint32_t size;
    uint32_t direct_blocks[8];

}  inode_t;

typedef struct
{
  char name[FS_MAX_FILENAME];
  uint32_t inode;

} dir_entry_t;

void fs_init(void);
void fs_list(void);
int fs_create(const char *name);
int fs_open(const char *name);
int fs_read(int fd, void *buffer, uint32_t size);
int fs_write(int fd , const void *buffer, uint32_t size);
void fs_close(int fd);
int fs_unlink(const char *name);




#endif