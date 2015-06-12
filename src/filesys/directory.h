#ifndef FILESYS_DIRECTORY_H
#define FILESYS_DIRECTORY_H

#include <stdbool.h>
#include <stddef.h>
#include "devices/disk.h"
#include "filesys/off_t.h"
#include "filesys/file.h"

/* Maximum length of a file name component.
   This is the traditional UNIX maximum length.
   After directories are implemented, this maximum length may be
   retained, but much longer full path names must be allowed. */
#define NAME_MAX 14

struct inode;

/* A directory. */
struct dir 
{
    struct inode *inode;                /* Backing store. */
    off_t pos;                          /* Current position. */
};

/* A single directory entry. */
struct dir_entry 
{
    disk_sector_t inode_sector;         /* Sector number of header. */
    char name[NAME_MAX + 1];            /* Null terminated file name. */
    bool is_dir;
    bool in_use;                        /* In use or free? */
};


/* Opening and closing directories. */
struct dir *dir_open (struct inode *);
struct dir *dir_open_root (void);
struct dir *dir_reopen (struct dir *);
void dir_close (struct dir *);
struct inode *dir_get_inode (struct dir *);

/* Reading and writing. */
bool dir_lookup (const struct dir *dir, const char *name, struct inode **inode, bool is_dir);
int dir_count (const struct dir *dir);
bool dir_add (struct dir *dir, const char *name, disk_sector_t inode_sector, bool is_dir);
bool dir_remove (struct dir *, disk_sector_t sec);

/* project 4 syscall */
bool dir_mkdir(char* path);
bool dir_chdir(char* path);
bool dir_readdir(struct file * file, char* name);

/* helper function */
// single component is zero or more than NAME_MAX
bool dir_is_valid(char*);
// dir exists either path or file
bool dir_is_path_exist(char*);
// dir is dir or file
bool dir_is_dir(char*);
void dir_set_dir_name_from_path(char*, char*);
void dir_set_upper_path_from_path(char*, char*);
disk_sector_t dir_get_sector_from_path(char *);

disk_sector_t dir_get_upper_sector_from_sector(disk_sector_t sec);





#endif /* filesys/directory.h */
