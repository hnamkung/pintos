#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include <hash.h>

struct hash frame_table;
struct hash page_table;

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

struct zombie {
        int tid;
        int exit_status;
        struct list_elem elem;
};

#endif /* userprog/process.h */
