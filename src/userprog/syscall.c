#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "threads/vaddr.h"

struct file* search_file(int fd);

void check_valid_addr(struct intr_frame *f, void *addr);

static void syscall_handler (struct intr_frame *);

static void syscall_halt(struct intr_frame *f);
static void syscall_exit(struct intr_frame *f);
static void syscall_exec(struct intr_frame *f);
static void syscall_wait(struct intr_frame *f);
static void syscall_create(struct intr_frame *f);
static void syscall_remove(struct intr_frame *f);
static void syscall_open(struct intr_frame *f);
static void syscall_filesize(struct intr_frame *f);
static void syscall_read(struct intr_frame *f);
static void syscall_write(struct intr_frame *f);
static void syscall_seek(struct intr_frame *f);
static void syscall_tell(struct intr_frame *f);
static void syscall_close(struct intr_frame *f);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

struct file* search_file(int fd)
{
    if(fd < 2 || fd > MAX_FD)
        return NULL;
    struct thread *t = thread_current();
    if(t->fd_table[fd].fd != -1) {
        return t->fd_table[fd].file;
    }
    return NULL;
}

void check_valid_addr(struct intr_frame *f, void *addr)
{
    struct thread *t = thread_current();
    if(is_kernel_vaddr(addr) || pagedir_get_page(t->pagedir, addr) == NULL) {
        *(int *)(f->esp+4) = -1;
        syscall_exit(f);
    }
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
    int SYS_NUM = *(int *)f->esp;
    if(SYS_NUM == SYS_WRITE) {
         syscall_write(f);
    }
    else if(SYS_NUM == SYS_HALT) {
        syscall_halt(f);
    }
    else if(SYS_NUM == SYS_EXIT) {
        syscall_exit(f);
    }
    else if(SYS_NUM == SYS_EXEC) {
        syscall_exec(f);
    }
    else if(SYS_NUM == SYS_WAIT) {
        syscall_wait(f);
    }
    else if(SYS_NUM == SYS_CREATE) {
        syscall_create(f);
    }
    else if(SYS_NUM == SYS_REMOVE) {
        syscall_remove(f);
    }
    else if(SYS_NUM == SYS_OPEN) {
        syscall_open(f);
    }
    else if(SYS_NUM == SYS_FILESIZE) {
        syscall_filesize(f);
    }
    else if(SYS_NUM == SYS_READ) {
        syscall_read(f);
    }
    else if(SYS_NUM == SYS_WRITE) {
        syscall_write(f);
    }
    else if(SYS_NUM == SYS_SEEK) {
        syscall_seek(f);
    }
    else if(SYS_NUM == SYS_TELL) {
        syscall_tell(f);
    }
    else if(SYS_NUM == SYS_CLOSE) {
        syscall_close(f);
    }
    else {
        thread_exit ();
    }
}
///////////////// exit
static void syscall_exit(struct intr_frame *f)
{
    int status = *(int *)(f->esp+4);
    struct thread *t = thread_current();
    printf("%s: exit(%d)\n", t->name, status);
    t->exit_status = status;
    thread_exit();
}

static void syscall_halt(struct intr_frame *f)
{
    power_off();
}

///////////////// exec
static void syscall_exec(struct intr_frame *f)
{
    char* cmd_line = *(void **)(f->esp+4);

    check_valid_addr(f, cmd_line);

    f->eax = process_execute(cmd_line);
}


////////////////// wait
static void syscall_wait(struct intr_frame *f)
{
    int tid = *(int *)(f->esp+4);
    f->eax = process_wait(tid);
}

static void syscall_create(struct intr_frame *f)
{
    char *name = *(char **)(f->esp+4);
    unsigned size = *(unsigned *)(f->esp+8);

    check_valid_addr(f, name);

    f->eax = filesys_create(name, size);
}

static void syscall_remove(struct intr_frame *f)
{
    char *name = *(char **)(f->esp+4);
    f->eax = filesys_remove(name);
}

static void syscall_open(struct intr_frame *f)
{
    char *name = *(char **)(f->esp+4);
    struct file *file;
    struct thread *t = thread_current();

    check_valid_addr(f, name);
    
    file = filesys_open(name);

    if(file == NULL) {
        f->eax = -1;
        return;
    }
    int fd;
    for(fd=2; fd<MAX_FD; fd++) {
        if(t->fd_table[fd].fd == -1) {
            t->fd_table[fd].file = file;
            t->fd_table[fd].fd = fd;
            break; 
        }
    }
    f->eax = fd;
}
static void syscall_filesize(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    struct file *file = search_file(fd);
    if(file == NULL) {
        return;
    }
    f->eax = file_length(file);
}

static void syscall_read(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    void *buffer = *(void **)(f->esp+8);
    unsigned size = *(unsigned *)(f->esp+12);
    
    check_valid_addr(f, buffer);

    if(fd == 0) {
        //f->eax = input_getc(); 
    }
    else {
        struct file *file = search_file(fd); 
        if(file == NULL) {
            f->eax = -1;
            return;
        }
        f->eax = file_read(file, buffer, size); 
    }
}

static void syscall_write(struct intr_frame *f)
{
    int fd = *(int *)(f->esp + 4);
    void *buffer = *(void **)(f->esp + 8);
    unsigned size = *(unsigned *)(f->esp + 12);

    check_valid_addr(f, buffer);
    
    if(fd == 1) {
        putbuf(buffer, size);
        f->eax = size;
        return;
    }
    struct file *file = search_file(fd);
    if(file == NULL)
        return;
    f->eax = file_write(file, buffer, size);
}


static void syscall_seek(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    unsigned position = *(unsigned *)(f->esp+8);
    struct file *file = search_file(fd); 
    if(file == NULL)
        return;
    file_seek(file, position);
    return;
}

static void syscall_tell(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    struct file *file = search_file(fd); 
    if(file == NULL)
        return;
    f->eax = file_tell(file);
}

static void syscall_close(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    struct file *file = search_file(fd); 
    if(file == NULL)
        return;
    struct thread *t = thread_current();
    t->fd_table[fd].fd = -1;
}

