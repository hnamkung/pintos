#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"

struct bitmap;
struct inode;
struct inode_disk;
struct single_disk;
struct double_disk;


void inode_init (void);
bool inode_create (disk_sector_t, off_t);
struct inode *inode_open (disk_sector_t);
struct inode *inode_reopen (struct inode *);
disk_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

disk_sector_t get_sector(struct inode_disk *disk_inode, off_t ith_sector);
void extend_file_if_necessary(struct inode_disk *disk_inode, off_t length);

struct single_disk * single_read_or_create(disk_sector_t list[], int j, uint8_t *bounce);
void single_writeback_or_alloc(disk_sector_t list[], int j, struct single_disk * disk_single);
struct double_disk * double_read_or_create(disk_sector_t double_elem, uint8_t *bounce);
void double_writeback_or_alloc(disk_sector_t double_elem, struct double_disk * disk_double);

disk_sector_t alloc_sector(void);
disk_sector_t alloc_single(struct single_disk *disk_single);
disk_sector_t alloc_double(struct double_disk *disk_double);

#endif /* filesys/inode.h */
