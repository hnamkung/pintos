#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "filesys/file.h"
#include "threads/palloc.h"
#include "devices/disk.h"
#include "vm/frame.h"
#include <hash.h>
#include "lib/kernel/bitmap.h"

struct hash swap_table;
struct bitmap *swap_bitmap;

struct swap
{
    int tid;
    uint8_t* vpage;
    disk_sector_t sector;
    struct hash_elem h_elem;
};

void swap_init();
struct frame * swap_read(struct page *p);
void swap_write(struct frame *f);
struct swap * swap_search(uint8_t* vpage, int tid);
void thread_exit_free_swaps();

#endif
