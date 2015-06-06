#include "filesys/cache.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include <string.h>

static struct cache cache_list[MAX_CACHE_SIZE];
static int clock_idx;
static struct lock cache_lock;

void cache_init()
{
    int i;
    for(i=0; i<MAX_CACHE_SIZE; i++) {
        memset (cache_list[i].buffer, 0, DISK_SECTOR_SIZE);
        cache_list[i].assigned = false;
        cache_list[i].access = false;
        cache_list[i].write = false;
    }
    clock_idx = 0;
    lock_init(&cache_lock);
}

void cache_write (disk_sector_t sec_no, const void *buffer)
{
    lock_acquire(&cache_lock);
    //printf("cache_write (%d)\n", sec_no);
    int i = get_sector_in_cache(sec_no);
    if(i == -1) {
        //printf("miss\n\n");
        i = get_cache();
        memcpy(cache_list[i].buffer, buffer, DISK_SECTOR_SIZE);
        cache_list[i].sec_no = sec_no;
        cache_list[i].assigned = true;
        cache_list[i].access = true;
        cache_list[i].write = true;
    } else {
        //printf("hit\n\n");
        memcpy(cache_list[i].buffer, buffer, DISK_SECTOR_SIZE);
        cache_list[i].access = true;
        cache_list[i].write = true;
    }
    lock_release(&cache_lock);
}

void cache_read (disk_sector_t sec_no, void *buffer)
{
    lock_acquire(&cache_lock);
    //printf("cache_read (%d)\n", sec_no);
    int i = get_sector_in_cache(sec_no);
    if(i == -1) {
        //printf("miss\n\n");
        i = get_cache();
        disk_read (filesys_disk, sec_no, cache_list[i].buffer);
        memcpy(buffer, cache_list[i].buffer, DISK_SECTOR_SIZE);
        cache_list[i].sec_no = sec_no;
        cache_list[i].assigned = true;
        cache_list[i].access = true;
    } else {
        //printf("hit\n\n");
        memcpy(buffer, cache_list[i].buffer, DISK_SECTOR_SIZE);
        cache_list[i].access = true;
    }
    lock_release(&cache_lock);
}

int get_sector_in_cache(disk_sector_t sec_no)
{
    int i;
    for(i=0; i<MAX_CACHE_SIZE; i++) {
        if(cache_list[i].assigned && cache_list[i].sec_no == sec_no) {
            return i;
        }
    }
    return -1;
}

int get_cache()
{
    int i;
    for(i=0; i<MAX_CACHE_SIZE; i++) {
        if(cache_list[i].assigned == false) {
            return i;
        }
    }

    return evict_cache();
}

int evict_cache()
{
    int i = clock_idx;
    while(true) {
        if(cache_list[i].access == false) {
            // clock_idx + 1
            clock_idx = i+1;
            if(clock_idx >= MAX_CACHE_SIZE)
                clock_idx = 0;
            
            // evict this frame
            disk_write (filesys_disk, cache_list[i].sec_no, cache_list[i].buffer); 

            // return evicted index
            return i;
        }
        cache_list[i].access = false;
        i++;
        if(i >= MAX_CACHE_SIZE)
            i = 0;
    }
}

void cache_close(void)
{
    lock_acquire(&cache_lock);
    int i;
    for(i=0; i<MAX_CACHE_SIZE; i++) {
        if(cache_list[i].assigned == true && cache_list[i].write == true) {
            disk_write (filesys_disk, cache_list[i].sec_no, cache_list[i].buffer); 
        }
    }
    lock_release(&cache_lock);
}


