#include "fs.h"
#include "vga.h"

extern uint8_t *ramdisk_get(void);
extern uint32_t ramdisk_size(void);

static uint8_t *fs_disk;

#define SUPERBLOCK_BLOCK 0
#define BLOCK_BITMAP_BLOCK 2
#define INODE_BITMAP_BLOCK 3
#define INODE_TABLE_BLOCK 4
#define ROOT_DIRECTORY_BLOCK 1

static superblock_t *superblock;
static uint8_t *block_bitmap;
static uint8_t *inode_bitmap;
static inode_t *inode_table;
static dir_entry_t *root_directory;

#define MAX_OPEN_FILES 8

static int open_files[MAX_OPEN_FILES];

void fs_init(void)
{
    uint32_t i;

    fs_disk = ramdisk_get();

    superblock = (superblock_t *)(fs_disk + SUPERBLOCK_BLOCK * FS_BLOCK_SIZE);
    block_bitmap = fs_disk + BLOCK_BITMAP_BLOCK * FS_BLOCK_SIZE;
    inode_bitmap = fs_disk + INODE_BITMAP_BLOCK * FS_BLOCK_SIZE;
    inode_table = (inode_t *)(fs_disk + INODE_TABLE_BLOCK * FS_BLOCK_SIZE);
    root_directory = (dir_entry_t *)(fs_disk + ROOT_DIRECTORY_BLOCK * FS_BLOCK_SIZE);

    for(i=0; i < FS_BLOCK_BITMAP_SIZE; i++)
    {
        block_bitmap[i] = 0;
    }
     for(i=0; i < FS_INODE_BITMAP_SIZE; i++)
    {
        inode_bitmap[i] = 0;
    }
    superblock->magic = FS_MAGIC;
    superblock->block_count = FS_TOTAL_BLOCKS;
    superblock->inode_count = FS_MAX_INODES;

    inode_bitmap[0] = 1;
    block_bitmap[0] = 1;
    block_bitmap[1] = 1;
    block_bitmap[2] = 1;
    block_bitmap[3] = 1;
    block_bitmap[4] = 1;


  inode_table = (inode_t *)((uint8_t *)fs_disk + (INODE_TABLE_BLOCK * FS_BLOCK_SIZE));
   inode_table[0].size = 0;
   inode_table[0].direct_blocks[0]=0;

   inode_table[0].size = 0;
   inode_table[0].direct_blocks[0] = 0;
   inode_table[0].direct_blocks[1] = 0;
   inode_table[0].direct_blocks[2] = 0;
   inode_table[0].direct_blocks[3] = 0;
   inode_table[0].direct_blocks[4] = 0;
   inode_table[0].direct_blocks[5] = 0;
   inode_table[0].direct_blocks[6] = 0;
   inode_table[0].direct_blocks[7] = 0;


    for(i = 0; i < FS_BLOCK_SIZE / sizeof(dir_entry_t); i++)
    {
        root_directory[i].inode = 0xFFFFFFFF;
    }
    for(i=0; i<MAX_OPEN_FILES;i++)
    {
        open_files[i] = -1;
    }

    (void)ramdisk_size();
}

int fs_open(const char *name)
{
    uint32_t i;

    for(i=0; i < FS_BLOCK_SIZE / sizeof(dir_entry_t);i++)
    {
        if(root_directory[i].inode != 0xFFFFFFFF)
        {
            uint32_t j =0;

            while(name[j] != '\0' && name[j] == root_directory[i].name[j])
            {
                j++;
            }

            if(name[j] == '\0' && root_directory[i].name[j] == '\0')
            {
                uint32_t fd;

                for(fd = 0; fd < MAX_OPEN_FILES ; fd++)
                {
                    if(open_files[fd] == -1)
                    {
                        open_files[fd] = root_directory[i].inode;
                        return(int)fd;
                    }
                }
                return -1;
            }
        }
    }
    return -1;
}

void fs_close(int fd)
{
    if(fd >= 0 && fd < MAX_OPEN_FILES)
    {
        open_files[fd] = -1;
    }
}

int fs_read(int fd, void *buffer, uint32_t size)
{
    uint32_t inode_number;
    inode_t *inode;
    uint32_t bytes_read;
    uint32_t remaining;
    uint32_t block_number;
    uint32_t offset;
    uint8_t *source;
    uint8_t *destination;


    if(fd < 0 || fd >= MAX_OPEN_FILES)
    {
        return -1;
    }

    if(open_files[fd] == -1)
    {
        return -1;
    }

    inode_number = (uint32_t)open_files[fd];
    inode = &inode_table[inode_number];

    if(size > inode-> size)
    {
        size = inode->size;
    }

    destination = (uint8_t *)buffer;
    bytes_read = 0;
    remaining = size;

    while(remaining > 0)
    {
        block_number = inode->direct_blocks[0];
        offset = bytes_read;

        if(offset >= FS_BLOCK_SIZE)
        { return (int)bytes_read ;}

        source = fs_disk + block_number * FS_BLOCK_SIZE + offset;

        *destination = *source;

        destination++;
        bytes_read++;
        remaining--;
    }
    return (int)bytes_read;
}

int fs_write(int fd, const void *buffer, uint32_t size)
{
    uint32_t inode_number;
    inode_t *inode;
    uint32_t block_number;
    uint32_t i;
    const uint8_t *source;
    uint8_t *destination;

    if(fd < 0|| fd >= MAX_OPEN_FILES)
    {
        return -1;
    }

    if(open_files[fd] == -1)
    {
        return -1;
    }

    inode_number = (uint32_t)open_files[fd];
    inode = &inode_table[inode_number];

    if(size > FS_BLOCK_SIZE)
    {
        size = FS_BLOCK_SIZE;
    }

    if(inode->direct_blocks[0] == 0)
    {
        return -1;
    }

    block_number = inode->direct_blocks[0];
    source = (const uint8_t *)buffer;
    destination = fs_disk + block_number * FS_BLOCK_SIZE;

    for(i=0; i< size; i++)
    {
        destination[i] = source[i];
    }

    inode->size = size;
    return (int)size;

}

int fs_unlink(const char *name)
{
    uint32_t i;

    for(i=0; i < FS_BLOCK_SIZE/ sizeof(dir_entry_t);i++)
    {
        if(root_directory[i].inode != 0xFFFFFFFF)
        {
            uint32_t j =0;
            while(name[j] != '\0' &&
                  root_directory[i].name[j] != '\0' &&
                  name[j] == root_directory[i].name[j])
                  {
                    j++;
                  }
                  if(name[j] == '\0' &&
                     root_directory[i].name[j] == '\0')
                     {
                        uint32_t inode_number = root_directory[i].inode;

                        inode_bitmap[inode_number] = 0;
                        root_directory[i].inode = 0xFFFFFFFF;
                        return 0;
                     }
        }
    }
    return -1;
}

int fs_create(const char *name)
{
    uint32_t inode_number;
    uint32_t directory_index;
    uint32_t i;

    for(inode_number = 1; inode_number < FS_MAX_INODES ; inode_number++)
    {
        if(inode_bitmap[inode_number] == 0)
        {
            break;
        }

    }

    if(inode_number >= FS_MAX_INODES)
    {
       return -1;
    }

    for(directory_index = 0;
        directory_index < FS_BLOCK_SIZE / sizeof(dir_entry_t);
         directory_index++)
    {
            if(root_directory[directory_index].inode == 0xFFFFFFFF)
            {
                break;
            }
    }
     if(directory_index >= FS_BLOCK_SIZE / sizeof(dir_entry_t))
     {
        return -1;
     }    

     inode_bitmap[inode_number] = 1;
     inode_table[inode_number].size = 0;
     for(i=0; i < 8; i++)
     {
        inode_table[inode_number].direct_blocks[i] = 0;

     }

     for(i=0 ; i < FS_MAX_FILENAME -1; i++)
     {
        root_directory[directory_index].name[i] = name[i];

        if(name[i] == '\0')
        {
            break;
        }
     }

     root_directory[directory_index].name[FS_MAX_FILENAME-1] = '\0';
     root_directory[directory_index].inode = inode_number;
     return 0;
}

void fs_list(void)
{
    uint32_t i;
    for(i=0 ; i< FS_BLOCK_SIZE / sizeof(dir_entry_t); i++)
    {
        if(root_directory[i].inode != 0xFFFFFFFF)
        {
            vga_puts(root_directory[i].name);
            vga_puts("\n");
        }
    }
}
    void fs_ls(void)
{

    for(int i=0; i < FS_MAX_INODES; i++)
    {
        if(root_directory[i].inode != 0)
        {
            vga_puts(root_directory[i].name);
            vga_puts("\n");
        }
    }
}

int fs_touch(const char *filename)
{
    for(int i = 0; i< FS_MAX_INODES;i++)
    {
        if(root_directory[i].inode != 0 && strcmp(root_directory[i].name, filename)==0)
        {
            vga_puts("Error : file already exists \n");
            return -1;
        }
    }
    int free_dir = -1;
    for(int i =0; i < FS_MAX_INODES; i++)
    {
        if(root_directory[i].inode == 0)
        {
            free_dir = i;
            break;
        }
    }

    if(free_dir == -1)
    return -1;

    inode_bitmap[free_inode] = 1;
    inode_table[free_inode].size = 0;
  strcpy(root_directory[free.dir].name,filename);
  root_directory[free_dir].inode = free_inode;
  return 0;
    
}

void fs_cat(const char *filename)
{
    int inode_idx = -1;
    for(int i=0; i < FS_MAX_INODES;i++)
    {
        if(root_directory[i].inode != 0 && strcmp(root_directory[i].name, filename)==0)
        {
            inode_idx = root_directory[i].inode;
            break;
        }
    }

    if(inode_idx == -1)
    {
        vga_puts("error: file not found\n");
        return;
    }
    inode_t *file_inode = &inode_table[inode_idx];
    if(file_inode->size == 0)
    {
        return;
    }

    uint32_t bytes_to_read = file_inode->size;
    for(int b =0; b< 8 && bytes_to_read > 0; b++)
    {
        uint32_t block_num = file_inode->direct_blocks[b];
        if(block_num == 0) break;

        char *block_ptr = (char *)(uint8_t *)fs_disk + (block_num*FS_BLOCK_SIZE);
        uint32_t chunk = (bytes_to_read > FS_BLOCK_SIZE) ? FS_BLOCK_SIZE : bytes_to_read;

        for(uint32_t j = 0 ; j < chunk; j++)
        {
            vga_putchar(block_ptr[j]);
        }
        bytes_to_read -= chunk;

    }
    vga_puts("\n");
}

int fs_rm(const char *filename)
{
    int dir_idx = -1;
    int inode_idx = -1;
    for(int i=0; i< FS_MAX_INODES;i++)
    {
     if(root_directory[i].inode != 0 && strcmp(root_directory[i].name, filename)==0)
     {
        dir_idx = i;
        inode_idx = root_directory[i].inode;
        break;
     }   
    }

    if(dir_idx == -1)
    {
        vga_puts("ERROR:file not found \n");
        return -1;
    }

    inode_t *file_inode = &inode_table[inode_idx];
    for(int b =0; b< 8; b++)
    {
        uint32_t block_num = file_inode->direct_blocks[b];
        if(block_num != 0)
        {
            block_bitmap[block_num] = 0;
            file_inode->direct_blocks[b] = 0;
        }
    }

    file_inode->size =0;
    inode->bitmap[inode_idx] = 0;

    root_directory[dir_idx].name[0] = '\0';
    root_directory[dir_idx].inode = 0;

    return 0;
}