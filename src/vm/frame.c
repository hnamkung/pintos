#include "vm/frame.h"

void frame_table_init()
{
    list_init(&frame_table);
}

void* frame_alloc(uint8_t* vpage, enum palloc_flags flag)
{
    uint8_t* ppage = palloc_get_page(flag);
    struct thread *t = thread_current();
    return ppage;
}

void frame_free(void* ppage)
{
}

