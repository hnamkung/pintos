#include "vm/frame.h"

void frame_table_init()
{
    list_init(&frame_table);
}

void* frame_alloc(uint8_t* vpage, enum palloc_flags flag)
{
    uint8_t* ppage = palloc_get_page(flag);
    struct thread *t = thread_current();

    // set up frame table
    struct frame *f = malloc(sizeof(struct frame));
    f->tid = t->tid;
    f->vpage = vpage;
    f->ppage = ppage;
    list_push_back(&frame_table, &f->l_elem);

    // set up page table
    struct page *p = malloc(sizeof(struct page));  
    p->tid = t->tid;
    p->vpage = vpage;
    p->f = f;
    hash_insert(&t->page_table, &p->h_elem);

    return ppage;
}

void frame_free(void* ppage)
{
}

