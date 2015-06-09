#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"
#include "filesys/cache.h"
#include "threads/thread.h"

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
    void
filesys_init (bool format) 
{
    filesys_disk = disk_get (0, 1);
    if (filesys_disk == NULL)
        PANIC ("hd0:1 (hdb) not present, file system initialization failed");

    cache_init();
    inode_init ();
    free_map_init ();

    if (format) 
        do_format ();

    free_map_open ();
    struct thread * t = thread_current();
    t->cur_dir = dir_open_root();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
    void
filesys_done (void) 
{
    inode_done();
    cache_close();
    free_map_close ();
}

/* Formats the file system. */
    static void
do_format (void)
{
    printf ("Formatting file system...");
    free_map_create ();

    if(!inode_create (ROOT_DIR_SECTOR, 0))
        PANIC ("root directory creation failed");

    struct inode * inode = inode_open(ROOT_DIR_SECTOR);

    struct dir_entry e;
    e.inode_sector = ROOT_DIR_SECTOR;
    e.name[0] = '.'; e.name[1] = 0;
    e.is_dir = true;
    e.in_use = true;
    if(!inode_write_at (inode, &e, sizeof e, inode_length(inode)) == sizeof e) {
        printf("init fail - filesys.c 136\n\n");
        ASSERT(false);
    }

    inode_close(inode);

    free_map_close ();
    printf ("done.\n");
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
    bool
filesys_create (char *path, off_t initial_size) 
{
    disk_sector_t new_sector = 0;
    disk_sector_t upper_dir_sector = 0;


    if(!dir_is_valid(path))
        return false;

    char dir_name[NAME_MAX+1];
    char *upper_dir_path = malloc(sizeof(strlen(path)+1));

    dir_set_dir_name_from_path(dir_name, path);
    dir_set_upper_path_from_path(upper_dir_path, path);

    if(!dir_is_path_exist(upper_dir_path) || !dir_is_dir(upper_dir_path))
        return false;

    upper_dir_sector = dir_get_sector_from_path(upper_dir_path);


    bool success = true;

    if(success && !free_map_allocate (1, &new_sector))
        success = false;

    if(success && !inode_create (new_sector, initial_size))
        success = false;

    struct dir * dir;
    dir = dir_open(inode_open(upper_dir_sector));
    success = success & dir_add(dir, dir_name, new_sector, false);
    dir_close(dir);


    if (!success && new_sector != 0) 
        free_map_release (new_sector, 1);

    return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
    struct file *
filesys_open (char *path)
{
    if(!dir_is_valid(path) || !dir_is_path_exist(path))
        return NULL;
    disk_sector_t sector = dir_get_sector_from_path(path);
    struct file * file = file_open(inode_open(sector));
    if(dir_is_dir(path)) {
        file->is_dir = true;
    }
    else 
        file->is_dir = false;

    return file;
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
    bool
filesys_remove (char *path) 
{
    if(!dir_is_valid(path) || !dir_is_path_exist(path))
        return false;

    disk_sector_t now = dir_get_sector_from_path(path);
    disk_sector_t upper = dir_get_upper_sector_from_sector(now);
    if(upper == -1) // when this is root dir
        return false;


    if(dir_is_dir(path)) {
        struct dir * now_dir = dir_open(inode_open(now));
        if(dir_count(now_dir) != 2) {
            dir_close(now_dir);
            return false;
        }
        dir_close(now_dir);

    }
    struct dir * upper_dir = dir_open(inode_open(upper));
    bool suc = dir_remove(upper_dir, now);
    dir_close(upper_dir);
    return suc;
}
