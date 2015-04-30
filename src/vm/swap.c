#include "vm/swap.h"

unsigned swap_hash(const struct hash_elem *e, void *aux)
{
    const struct swap *s = hash_entry(e, struct swap, h_elem);
    void *addr = s->vpage + s->tid;
    return hash_bytes(&addr, sizeof(addr));
}

bool swap_less(struct hash_elem *aa, const struct hash_elem *bb, void *aux)
{
    const struct swap *a = hash_entry(aa, struct swap, h_elem);
    const struct swap *b = hash_entry(bb, struct swap, h_elem);
    return (a->vpage == b->vpage) ? (a->tid < b->tid) : (a->vpage < b->vpage);
}

void swap_init()
{
    hash_init(&swap_table, swap_hash, swap_less, NULL);

    swap_bitmap = bitmap_create(disk_size(disk_get(1, 1)));
    bitmap_set_all(swap_bitmap, false);
}

struct swap * swap_search(uint8_t* vpage, int tid)
{
    struct swap temp_swap;
    temp_swap.tid = tid;
    temp_swap.vpage = vpage;
    struct hash_elem *e = hash_find(&swap_table, &temp_swap.h_elem);
    if(e == NULL)
        return e;
    return hash_entry(e, struct swap, h_elem);
}

// called from page fault, exception.c
struct frame * swap_read(uint8_t *vpage)
{
    struct thread *t = thread_current();
    
    uint8_t* ppage = palloc_evict_if_necessary(PAL_USER | PAL_ZERO);
    //printf("3. swap in] %p -> %p\n", vpage, ppage);

    struct swap *s;
    s = swap_search(vpage, t->tid);

    int count =0;
    while(count < PGSIZE / DISK_SECTOR_SIZE) {
        disk_read(disk_get(1,1), s->sector + count, ppage + count * DISK_SECTOR_SIZE);
        count++;
    }
    bitmap_set_multiple(swap_bitmap, s->sector, PGSIZE/DISK_SECTOR_SIZE, false);

    hash_delete(&swap_table, &s->h_elem);
    free(s);

    struct frame *f = malloc(sizeof(struct frame));
    f->tid = t->tid;
    f->vpage = vpage;
    f->ppage = ppage;
    list_push_back(&frame_table, &f->l_elem);

    return f;
}

void swap_write(struct frame *f)
{
    //printf("2. swap out] %p -> %p\n", f->vpage, f->ppage);

    size_t start = bitmap_scan_and_flip(swap_bitmap, 0, PGSIZE/DISK_SECTOR_SIZE, false);

    struct swap *s = malloc(sizeof(struct swap));
    int count = 0;
    s->tid = f->tid;
    s->vpage = f->vpage;
    s->sector = start;

    hash_insert(&swap_table, &s->h_elem);
    while(count < PGSIZE / DISK_SECTOR_SIZE)
    {
        disk_write(disk_get(1, 1), start + count, f->ppage + count * DISK_SECTOR_SIZE);
        count++;
    }
}

void thread_exit_free_swaps()
{
    return;
    struct thread * t = thread_current();
    int tid = t->tid;
    struct swap *s;
    struct hash_iterator i;

    if(hash_empty(&swap_table))
        return;

    hash_first(&i, &swap_table);

    while(hash_next(&i)) {
        s = hash_entry(hash_cur(&i), struct swap, h_elem);
        hash_delete(&swap_table, &s->h_elem);
        hash_first(&i, &swap_table);

    }
}

