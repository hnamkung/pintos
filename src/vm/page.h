#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "threads/palloc.h"
#include "vm/frame.h"
#include <hash.h>
#include "lib/kernel/hash.h"

struct page
{
    int tid;
    uint8_t* vpage;
    struct frame *f;
    bool valid; 
    struct hash_elem h_elem;
};

void page_table_init(struct hash * page_table);
struct page * page_search(uint8_t *vpage);
void thread_exit_free_pages();

#endif
