#include "userprog/process.h"
#include "threads/palloc.h"
#include <hash.h>

struct frame
{
    tid_t owner;
    struct hash_elem h_elem;
    struct list_elem l_elem;
};

void frame_init();
void* frame_alloc(uint8_t* vaddr, enum palloc_flags flag);
void frame_free(void* paddr);


