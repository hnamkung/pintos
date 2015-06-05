#include "vm/page.h"

unsigned page_hash(const struct hash_elem *e, void *aux);
bool page_less(const struct hash_elem *aa, const struct hash_elem *bb, void *aux);

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


void page_table_init(struct hash * page_table)
{
    hash_init(page_table, page_hash, page_less, NULL);
}

struct page * page_alloc(uint8_t *vpage) 
{
//    printf("page alloc : %p\n\n", vpage);
    struct page *p = malloc(sizeof(struct page));  
    struct thread *t = thread_current();
    p->tid = t->tid;
    p->vpage = vpage;
    hash_insert(&t->page_table, &p->h_elem);

    return p;
}
struct page * page_search(uint8_t *vpage)
{
    struct thread *t = thread_current();
    struct page temp_page;
    temp_page.vpage = vpage;
    struct hash_elem *e = hash_find(&t->page_table, &temp_page.h_elem);
    if(e == NULL)
        return NULL;
    return hash_entry(e, struct page, h_elem);
}

void page_free(struct page * p)
{
    struct thread * t = thread_current();
    hash_delete(&t->page_table, &p->h_elem);
    free(p);
}


void thread_exit_free_pages()
{
    struct thread * t = thread_current();
    //int tid = t->tid;
    struct hash page_table = t->page_table;
    struct page *p;
    struct hash_iterator i;

    if(hash_empty(&page_table))
        return;
    
    hash_first(&i, &page_table);

    while(hash_next(&i)) {
        p = hash_entry(hash_cur(&i), struct page, h_elem);
        if(p->state == MMAP_LOADED) {
            mmap_write(p);
        }
        hash_delete(&page_table, &p->h_elem);
        hash_first(&i, &page_table);
        free(p);
    }
}

