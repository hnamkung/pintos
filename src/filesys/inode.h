#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include <list.h>
#include "filesys/off_t.h"
#include "devices/disk.h"

#define DIRECT_COUNT 10
#define SINGLE_INDIRECT_COUNT 10
#define UNUSEDL (128 - 1 - DIRECT_COUNT - SINGLE_INDIRECT_COUNT - 2)
#define INDIRECT_COUNT 128
#define NOT_ALLOCATED 9999999


struct bitmap;
struct inode;
struct inode_disk;
struct single_disk;
struct double_disk;

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
{
    off_t length;

    disk_sector_t direct_list[DIRECT_COUNT];
    disk_sector_t single_list[SINGLE_INDIRECT_COUNT];
    disk_sector_t double_elem;

    unsigned magic;                     /* Magic number. */
    uint32_t unused[UNUSEDL];               /* Not used. */
};

struct single_disk
{
    disk_sector_t direct_list[INDIRECT_COUNT];
};

struct double_disk
{
    disk_sector_t single_list[INDIRECT_COUNT];
};

/* In-memory inode. */
struct inode 
{
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
};


void inode_init (void);
bool inode_create (disk_sector_t, off_t);
struct inode *inode_open (disk_sector_t);
void inode_done(void);
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
void extend_file_if_necessary(disk_sector_t sec_no, struct inode_disk *disk_inode, off_t length);

struct single_disk * single_read_or_create(disk_sector_t list[], int j, uint8_t *bounce);
void single_writeback_or_alloc(disk_sector_t list[], int j, struct single_disk * disk_single);
struct double_disk * double_read_or_create(disk_sector_t double_elem, uint8_t *bounce);
void double_writeback_or_alloc(disk_sector_t double_elem, struct double_disk * disk_double);

disk_sector_t alloc_sector(void);
disk_sector_t alloc_single(struct single_disk *disk_single);
disk_sector_t alloc_double(struct double_disk *disk_double);
    

#endif /* filesys/inode.h */
