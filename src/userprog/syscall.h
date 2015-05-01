#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void syscall_exit(struct intr_frame *f);

struct lock file_lock;

struct file_fd
{
    struct file *file;
    int fd;
};

#endif /* userprog/syscall.h */
