#include "vm/frame.h"

void frame_table_init()
{
    list_init(&frame_table);
    lock_init(&frame_lock);
}

uint8_t* frame_alloc(struct page *p, enum palloc_flags flag)
{
    uint8_t* ppage = palloc_evict_if_necessary(flag);
    //printf("%d] 1. frame alloc] %p -> %p\n", thread_current()->tid, p->vpage, ppage);

    struct thread *t = thread_current();
    
    // set up frame table
    struct frame *f = malloc(sizeof(struct frame));
    f->tid = t->tid;
    f->p = p;
    f->vpage = p->vpage;
    f->ppage = ppage;
    f->pagedir = t->pagedir;
    list_push_back(&frame_table, &f->l_elem);

    return ppage;
}
uint8_t* palloc_evict_if_necessary(enum palloc_flags flag)
{
    uint8_t* ppage = palloc_get_page(flag);
    if(ppage == NULL) {
        evict_frame();
        ppage = palloc_get_page(flag | PAL_ASSERT);
    }
    return ppage;
}


void thread_exit_free_frames()
{
    struct thread * t = thread_current();
    int tid = t->tid;
    struct frame *f;
    struct list_elem *e;

    for(e = list_begin(&frame_table); e != list_end(&frame_table); ) {
        f = list_entry(e, struct frame, l_elem);
        if(f->tid == tid) {
            e = list_remove(e);
            free(f);
        } else {
            e = list_next(e);
        }
    }
}

// private functions
void evict_frame()
{
    struct thread *t = thread_current();
    // get victim_frame
    struct frame *victim_f = list_entry(list_pop_front(&frame_table), struct frame, l_elem);

    struct page *p = victim_f->p; 
    uint8_t* vpage = victim_f->vpage;
    uint8_t* ppage = victim_f->ppage;
    uint32_t* pagedir = victim_f->pagedir;

    if(p->state == IN_PHYS_MEMORY) {
        // victim_frame -> disk
        swap_write(victim_f);
    }
    else if(p->state == MMAP_LOADED) {
        // victim_frame -> file
        //printf("2. evict loaded mmap\n");
        mmap_write(victim_f->p);
    }
    // free frame and clear page table
    pagedir_clear_page(pagedir, victim_f->vpage);
    palloc_free_page(ppage);

    // free frame
    list_remove(&victim_f->l_elem);
    free(victim_f);
}

