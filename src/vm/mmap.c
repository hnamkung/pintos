#include "vm/mmap.h"


void mmap_table_init(struct list * mmap_table)
{
    list_init(mmap_table);
}

// called from page fault, exception.c
struct frame * mmap_read(struct page *p)
{
    p->state = MMAP_LOADED;

    struct thread *t = thread_current();
    
    uint8_t* vpage = p->vpage;
    uint8_t* ppage = palloc_evict_if_necessary(PAL_USER | PAL_ZERO);

    off_t diff = p->mmap_end_offset - p->mmap_start_offset;
    file_seek(p->file, p->mmap_start_offset);
    file_read(p->file, ppage, diff);
    memset (ppage + diff, 0, (PGSIZE - diff));
    
    struct frame *f = malloc(sizeof(struct frame));
    f->tid = t->tid;
    f->p = p;
    f->vpage = vpage;
    f->ppage = ppage;
    f->pagedir = t->pagedir;
    list_push_back(&frame_table, &f->l_elem);

    return f;
}

void mmap_write(struct page *p)
{
    p->state = MMAP_NOT_LOADED;
    //printf("vpage(%p) start(%d) diff(%d) p->file(%p) \n\n", p->vpage, p->mmap_start_offset, p->mmap_end_offset - p->mmap_start_offset, p->file);
    if(pagedir_is_dirty(thread_current()->pagedir, p->vpage)) {
        file_seek(p->file, p->mmap_start_offset);
        file_write(p->file, p->vpage, p->mmap_end_offset - p->mmap_start_offset);
    }
}
