#include "vm/mmap.h"


void mmap_table_init(struct list * mmap_table)
{
    list_init(mmap_table);
}

// called from page fault, exception.c
struct frame * mmap_read(struct page *p)
{
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
    file_seek(p->file, p->mmap_start_offset);
    file_write(p->file, p->vpage, p->mmap_end_offset - p->mmap_start_offset);
}

void mmap_munmap(struct mmap *m)
{
    struct file* file = m->file;
    uint8_t* start_addr = m->start_addr;
    off_t file_len = file_length(file);    
    
    off_t offset = 0;
    while(offset < file_len) {
        struct page *p = page_search(start_addr + offset); 
        if(p == NULL) {
            printf("should not happen!!\n\n");
            ASSERT(false);
        }
        if(p->state == MMAP_LOADED) {
            mmap_write(p);
        }
        page_free(p);
    }
}
