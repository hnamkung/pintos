#ifndef VM_MMAP_H
#define VM_MMAP_H

#include "filesys/file.h"
#include "threads/palloc.h"
#include "devices/disk.h"
#include "vm/frame.h"

struct list mmap_table;

struct mmap
{
    int mmap_id;
    struct file *file;
    uint8_t* start_addr;
    struct list_elem l_elem;
};

void mmap_table_init(struct list * mmap_table);
void mmap_write(struct page *p);
struct frame * mmap_read(struct page *p);

#endif
