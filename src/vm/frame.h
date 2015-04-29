#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/thread.h"
#include "vm/page.h"
#include "threads/vaddr.h"
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
uint8_t* frame_alloc(uint8_t* vpage, enum palloc_flags flag);
uint8_t* palloc_evict_if_necessary(enum palloc_flags flag);
void frame_free(void* ppage);
struct frame * frame_search(uint8_t* vpage);

// private functions
void evict_frame();

#endif 

