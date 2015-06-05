#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/palloc.h"
#include "threads/thread.h"
#include "vm/page.h"
#include "threads/vaddr.h"
#include <hash.h>
#include "userprog/pagedir.h"

struct list frame_table;
struct lock frame_lock;

struct frame
{
    int tid;

    struct page *p;
    uint8_t* vpage;
    uint8_t* ppage;
    uint32_t *pagedir;
    struct list_elem l_elem;
};

void frame_table_init(void);
uint8_t* frame_alloc(struct page *p, enum palloc_flags flag);
uint8_t* palloc_evict_if_necessary(enum palloc_flags flag);
void thread_exit_free_frames(void);

// private functions
void evict_frame(void);

#endif 

