#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "threads/palloc.h"
#include "vm/frame.h"
#include <hash.h>
#include "lib/kernel/hash.h"

enum page_state
{
    IN_PHYS_MEMORY,
    IN_SWAP_DISK,
    MMAP_LOADED,
    MMAP_NOT_LOADED
};

struct page
{
    int tid;
    uint8_t* vpage;

    enum page_state state;
    
    uint8_t* ppage; // if IN_PHYS_MEMORY
    // struct swap s; if IN_SWAP_DISK
    // int sector? if IN_FILE_DISK

    struct hash_elem h_elem;
};

void page_table_init(struct hash * page_table);
struct page * page_alloc(uint8_t *vpage);
struct page * page_search(uint8_t *vpage);
void thread_exit_free_pages();

#endif
