#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static void syscall_write(struct intr_frame *f);
static void syscall_exit(struct intr_frame *f);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
    int SYS_NUM = *(int *)f->esp;
    if(SYS_NUM == SYS_WRITE) {
         syscall_write(f);
    }
    else if(SYS_NUM == SYS_EXIT) {
        syscall_exit(f);
    }
    else {
        thread_exit ();
    }
}
static void syscall_write(struct intr_frame *f)
{
    int fd = *(int *)(f->esp + 4);
    char *buffer = *(char **)(f->esp + 8);
    int size = *(int *)(f->esp + 12);
    struct file *file;
    
    if(fd == 1) {
        putbuf(buffer, size);
        f->eax = size;
    }
}

static void syscall_exit(struct intr_frame *f)
{
    int status = *(int *)(f->esp+4);
    struct thread *t = thread_current();
    printf("%s: exit(%d)\n", t->name, status);
    thread_exit();
}
