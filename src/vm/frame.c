#include "vm/frame.h"

void frame_init()
{

}

void* frame_allocate(uint8_t* vpage, enum palloc_flags flag)
{
        uint8_t* kpage = palloc_get_page(flag);
        return kpage;
}

void frame_free(void* paddr)
{
}

