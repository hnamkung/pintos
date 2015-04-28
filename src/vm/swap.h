#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "threads/palloc.h"
#include "devices/disk.h"
#include <hash.h>

struct swap
{
    int tid;
    struct hash_elem elem;
};

void swap_init();

#endif
