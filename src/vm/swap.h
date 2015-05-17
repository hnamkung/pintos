#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "filesys/file.h"
#include "threads/palloc.h"
#include "devices/disk.h"
#include "vm/frame.h"
#include <hash.h>
#include "lib/kernel/bitmap.h"

struct bitmap *swap_bitmap;

void swap_init();
struct frame * swap_read(struct page *p);
void swap_write(struct frame *f);
void thread_exit_free_swaps();

#endif
