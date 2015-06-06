#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "devices/disk.h"
#include "filesys/filesys.h"
#include <stdbool.h>

#define MAX_CACHE_SIZE 64

struct cache
{
    disk_sector_t sec_no;
    char buffer[DISK_SECTOR_SIZE];
    bool assigned;

    // for replacement algorithm
    bool access;

    // for write-behind
    bool write;
};

void cache_init(void);
void cache_write (disk_sector_t sec_no, const void *buffer);
void cache_read (disk_sector_t sec_no, void *buffer);
int get_sector_in_cache(disk_sector_t sec_no);
int get_cache(void);
int evict_cache(void);
void cache_close(void);

#endif //FILESYS_CACHE_H

