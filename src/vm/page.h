#include "userprog/process.h"
#include "threads/palloc.h"
#include <hash.h>

struct page
{
    tid_t owner;
    struct frame *frame;
    struct hash_elem h_elem;
};

unsigned page_hash(const struct hash_elem *p_, void*aux);

