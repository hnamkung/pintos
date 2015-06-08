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
    cache_close();
    free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
    bool
filesys_create (char *path, off_t initial_size) 
{
    disk_sector_t inode_sector = 0;

    char dir_name[NAME_MAX+1];
    struct dir upper_dir;

    bool success = true;
    if(!dir_get_upper_and_name_from_path(&upper_dir, dir_name, path)) {
        success = false;
    }

    if(success && !free_map_allocate (1, &inode_sector))
        success = false;

    if(success && !inode_create (inode_sector, initial_size))
        success = false;

    if(success && !dir_add (&upper_dir, dir_name, inode_sector, false))
        success = false;

    if (!success && inode_sector != 0) 
        free_map_release (inode_sector, 1);
    //dir_close (dir);

    return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
    struct file *
filesys_open (char *name)
{
    char dir_name[NAME_MAX+1];
    struct dir upper_dir;

    struct inode *inode = NULL;

    if(!dir_get_upper_and_name_from_path(&upper_dir, dir_name, name)) {
        dir_lookup (&upper_dir, dir_name, &inode, false);
    }

    //dir_close (dir);

    return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
    bool
filesys_remove (char *name) 
{
    struct dir *dir = dir_open_root ();
    bool success = dir != NULL && dir_remove (dir, name);
    dir_close (dir); 

    return success;
}

/* Formats the file system. */
    static void
do_format (void)
{
    printf ("Formatting file system...");
    free_map_create ();

    if(!inode_create (ROOT_DIR_SECTOR, 0))
        PANIC ("root directory creation failed");

    free_map_close ();
    printf ("done.\n");
}
