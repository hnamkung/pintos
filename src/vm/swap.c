#include "vm/swap.h"

void swap_init()
{
    swap_bitmap = bitmap_create(disk_size(disk_get(1, 1)));
    bitmap_set_all(swap_bitmap, false);
}

// called from page fault, exception.c
struct frame * swap_read(struct page *p)
{
    p->state = IN_PHYS_MEMORY;

    struct thread *t = thread_current();
    
    uint8_t* vpage = p->vpage;
    uint8_t* ppage = palloc_evict_if_necessary(PAL_USER | PAL_ZERO);
    //printf("%d] 3. swap in] %p -> %p\n", thread_current()->tid, vpage, ppage);

    int count;
    for(count=0; count<(PGSIZE/DISK_SECTOR_SIZE); count++) {
        disk_read(disk_get(1,1), p->sector + count, ppage + count * DISK_SECTOR_SIZE);
    }

    bitmap_set_multiple(swap_bitmap, p->sector, PGSIZE/DISK_SECTOR_SIZE, false);

    struct frame *f = malloc(sizeof(struct frame));
    f->tid = t->tid;
    f->p = p;
    f->vpage = vpage;
    f->ppage = ppage;
    f->pagedir = t->pagedir;
    list_push_back(&frame_table, &f->l_elem);

    return f;
}

void swap_write(struct frame *f)
{
    
    f->p->state = IN_SWAP_DISK;
    //printf("%d] 2. swap out] %p -> %p\n", thread_current()->tid, f->vpage, f->ppage);

    size_t sector = bitmap_scan_and_flip(swap_bitmap, 0, PGSIZE/DISK_SECTOR_SIZE, false);
    if(sector == BITMAP_ERROR) {
        ASSERT(false);
    }

    f->p->sector = sector;

    int count;
    for(count=0; count<(PGSIZE/DISK_SECTOR_SIZE); count++) {
        disk_write(disk_get(1, 1), sector + count, f->ppage + count * DISK_SECTOR_SIZE);
    }
}

void thread_exit_free_swaps()
{

}

