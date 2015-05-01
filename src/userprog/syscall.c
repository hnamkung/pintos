#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "vm/page.h"

struct file* search_file(int fd);

void check_valid_addr(struct intr_frame *f, void *addr);

static void syscall_handler (struct intr_frame *);

static void syscall_halt(struct intr_frame *f);
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
    lock_init(&file_lock);
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
    if(is_kernel_vaddr(addr) || addr == NULL) {
        *(int *)(f->esp+4) = -1;
        syscall_exit(f);
    }
}


    static void
syscall_handler (struct intr_frame *f UNUSED) 
{
    struct thread *t = thread_current();
    void *addr = f->esp;
    if(is_kernel_vaddr(addr)) {
        printf("%s: exit(%d)\n", t->name, -1);
        t->exit_status = -1;
        thread_exit();
    } 

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

void syscall_exit(struct intr_frame *f)
{
    int status = *(int *)(f->esp+4);
    struct thread *t = thread_current();

    if(is_kernel_vaddr(f->esp+4))
        status = -1;

    printf("%s: exit(%d)\n", t->name, status);
    t->exit_status = status;
    thread_exit();
}

static void syscall_halt(struct intr_frame *f)
{
    power_off();
}

static void syscall_exec(struct intr_frame *f)
{
    char* cmd_line = *(void **)(f->esp+4);

    check_valid_addr(f, cmd_line);

    if(pagedir_get_page(thread_current()->pagedir, cmd_line) == NULL) {
        *(int *)(f->esp+4) = -1;
        syscall_exit(f);
    }

    f->eax = process_execute(cmd_line);
}


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

    lock_acquire(&file_lock);
    f->eax = filesys_create(name, size);
    lock_release(&file_lock);
}

static void syscall_remove(struct intr_frame *f)
{
    char *name = *(char **)(f->esp+4);
    lock_acquire(&file_lock);
    f->eax = filesys_remove(name);
    lock_release(&file_lock);
}

static void syscall_open(struct intr_frame *f)
{
    char *name = *(char **)(f->esp+4);
    struct file *file;
    struct thread *t = thread_current();

    check_valid_addr(f, name);

    lock_acquire(&file_lock);
    file = filesys_open(name);
    lock_release(&file_lock);

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
    lock_acquire(&file_lock);
    f->eax = file_length(file);
    lock_release(&file_lock);
}

static void syscall_read(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    void *buffer = *(void **)(f->esp+8);
    unsigned size = *(unsigned *)(f->esp+12);

    check_valid_addr(f, buffer);

    if(fd == 0) {
        f->eax = input_getc(); 
    }
    else {
        struct file *file = search_file(fd); 
        if(file == NULL) {
            f->eax = -1;
            return;
        }
        lock_acquire(&file_lock);
        f->eax = file_read(file, buffer, size); 
        lock_release(&file_lock);
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

    lock_acquire(&file_lock);
    f->eax = file_write(file, buffer, size);
    lock_release(&file_lock);
}


static void syscall_seek(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    unsigned position = *(unsigned *)(f->esp+8);
    struct file *file = search_file(fd); 
    if(file == NULL)
        return;
    lock_acquire(&file_lock);
    file_seek(file, position);
    lock_release(&file_lock);
    return;
}

static void syscall_tell(struct intr_frame *f)
{
    int fd = *(int *)(f->esp+4);
    struct file *file = search_file(fd); 
    if(file == NULL)
        return;
    lock_acquire(&file_lock);
    f->eax = file_tell(file);
    lock_release(&file_lock);
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

