#include "userprog/process.h"
#include "threads/palloc.h"
#include "devices/disk.h"
#include <hash.h>

struct swap
{
    tid_t owner;
    struct hash_elem elem;
};

void swap_init();
