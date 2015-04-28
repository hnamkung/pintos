#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/thread.h"
#include "vm/page.h"
#include <hash.h>

struct list frame_table;

struct frame
{
    int tid;
    uint8_t* vpage;
    uint8_t* ppage;
    struct list_elem l_elem;
};

void frame_table_init();
void* frame_alloc(uint8_t* vpage, enum palloc_flags flag);
void frame_free(void* ppage);

#endif 

