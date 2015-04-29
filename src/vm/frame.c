#include "vm/frame.h"

void frame_table_init()
{
    list_init(&frame_table);
}

uint8_t* frame_alloc(uint8_t* vpage, enum palloc_flags flag)
{
    uint8_t* ppage = palloc_evict_if_necessary(flag);

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
    p->valid = 1;
    hash_insert(&t->page_table, &p->h_elem);

    return ppage;
}
uint8_t* palloc_evict_if_necessary(enum palloc_flags flag)
{
    uint8_t* ppage = palloc_get_page(flag);
    if(ppage == NULL) {
        evict_frame();
        ppage = palloc_get_page(flag);

        // should not happen, panic the kernel
        if(ppage == NULL) {
            ASSERT(false);
        }
    }
    return ppage;
}


void frame_free(void* ppage)
{
}

struct frame * frame_search(uint8_t* vpage)
{
}


// private functions
void evict_frame()
{
    // get victim_frame
    struct frame *victim_f = list_entry(list_pop_front(&frame_table), struct frame, l_elem);

    // get victim_page
    struct page *victim_p = frame_search(victim_f->vpage);
    victim_p->f = NULL;
    victim_p->valid = 0;

    // clear page table
    pagedir_clear_page(thread_current()->pagedir, victim_f->vpage);
    
    // victim_frame -> disk
    swap_write(victim_f);

    // free frame
    free(victim_f);
}

