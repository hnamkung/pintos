#include "vm/page.h"

unsigned page_hash(const struct hash_elem *e, void *aux)
{
    const struct page *p = hash_entry(e, struct page, h_elem);
//    char buf[20];
//    sprintf(buf, "%d", &f->ppage);
    return hash_bytes(&p->vpage, sizeof p->vpage);
}

bool page_less(const struct hash_elem *aa, const struct hash_elem *bb, void *aux)
{
    const struct page *a = hash_entry(aa, struct page, h_elem);
    const struct page *b = hash_entry(bb, struct page, h_elem);
    return (a->vpage < b->vpage);
} 


void page_table_init(struct hash page_table)
{
    hash_init(&page_table, page_hash, page_less, NULL);
}

struct page * page_search(uint8_t *vpage)
{
    struct thread *t = thread_current();
    struct page temp_page;
    temp_page.vpage = vpage;
    struct hash_elem *e = hash_find(&t->page_table, &temp_page.h_elem);
    if(e == NULL)
        return e;
    return hash_entry(e, struct page, h_elem);
}


