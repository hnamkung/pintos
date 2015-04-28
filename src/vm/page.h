#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "threads/palloc.h"
#include <hash.h>

struct page
{
    int tid;
    uint8_t* vpage;
    struct hash_elem h_elem;
};

void page_table_init(struct hash page_table);



#endif
