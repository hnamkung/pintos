#include "filesys/directory.h"
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "filesys/free-map.h"

/* Opens and returns the directory for the given INODE, of which
   it takes ownership.  Returns a null pointer on failure. */
    struct dir *
dir_open (struct inode *inode) 
{
    struct dir *dir = calloc (1, sizeof *dir);
    if (inode != NULL && dir != NULL)
    {
        dir->inode = inode;
        dir->pos = 0;
        return dir;
    }
    else
    {
        inode_close (inode);
        free (dir);
        return NULL; 
    }
}

/* Opens the root directory and returns a directory for it.
   Return true if successful, false on failure. */
    struct dir *
dir_open_root (void)
{
    return dir_open (inode_open (ROOT_DIR_SECTOR));
}

/* Opens and returns a new directory for the same inode as DIR.
   Returns a null pointer on failure. */
    struct dir *
dir_reopen (struct dir *dir) 
{
    return dir_open (inode_reopen (dir->inode));
}

/* Destroys DIR and frees associated resources. */
    void
dir_close (struct dir *dir) 
{
    if (dir != NULL)
    {
        inode_close (dir->inode);
        free (dir);
    }
}

/* Returns the inode encapsulated by DIR. */
    struct inode *
dir_get_inode (struct dir *dir) 
{
    return dir->inode;
}

/* Searches DIR for a file with the given NAME.
   If successful, returns true, sets *EP to the directory entry
   if EP is non-null, and sets *OFSP to the byte offset of the
   directory entry if OFSP is non-null.
   otherwise, returns false and ignores EP and OFSP. */
    static bool
lookup (const struct dir *dir, const char *name,
        struct dir_entry *ep, off_t *ofsp, bool is_dir)
{
    struct dir_entry e;
    size_t ofs;

    ASSERT (dir != NULL);
    ASSERT (name != NULL);

    for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
            ofs += sizeof e) 
        if (e.is_dir == is_dir && e.in_use && !strcmp (name, e.name)) 
        {
            if (ep != NULL)
                *ep = e;
            if (ofsp != NULL)
                *ofsp = ofs;
            return true;
        }
    return false;
}

/* Searches DIR for a file with the given NAME
   and returns true if one exists, false otherwise.
   On success, sets *INODE to an inode for the file, otherwise to
   a null pointer.  The caller must close *INODE. */
bool dir_lookup (const struct dir *dir, const char *name, struct inode **inode, bool is_dir)
{
    struct dir_entry e;

    ASSERT (dir != NULL);
    ASSERT (name != NULL);

    if (lookup (dir, name, &e, NULL, is_dir))
        *inode = inode_open (e.inode_sector);
    else
        *inode = NULL;

    return *inode != NULL;
}

int dir_count (const struct dir *dir)
{
    struct dir_entry e;
    size_t ofs;
    int count = 0;

    for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) {
        if (e.in_use) 
        {
            count++;
        }
    }
    return count;
}

/* Adds a file named NAME to DIR, which must not already contain a
   file by that name.  The file's inode is in sector
   INODE_SECTOR.
   Returns true if successful, false on failure.
   Fails if NAME is invalid (i.e. too long) or a disk or memory
   error occurs. */
    bool
dir_add (struct dir *dir, const char *name, disk_sector_t inode_sector, bool is_dir)
{

    ASSERT (dir != NULL);
    ASSERT (name != NULL);

    /* Check NAME for validity. */
    if (*name == '\0' || strlen (name) > NAME_MAX) {
        return false;
    }

    /* Check that NAME is not in use. */
    if (lookup (dir, name, NULL, NULL, false) || lookup (dir, name, NULL, NULL, true)) {
        return false;
    }

    struct dir_entry e;
    int flag = 0;
    off_t ofs;

    for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) {
        if (e.in_use == false) {
            flag = 1;
            break;
        }
    }
    if(flag == 0) {
        ofs = inode_length(dir->inode);
    }

    /* Write slot. */
    e.inode_sector = inode_sector;
    strlcpy (e.name, name, sizeof e.name);
    e.is_dir = is_dir;
    e.in_use = true;
    if(!inode_write_at (dir->inode, &e, sizeof e, ofs) == sizeof e) {
        return false;
    }

    if(is_dir) {
        struct inode * inode = inode_open(inode_sector);

        e.inode_sector = inode_sector;
        e.name[0] = '.'; e.name[1] = 0;
        e.is_dir = true;
        e.in_use = true;
        if(!inode_write_at (inode, &e, sizeof e, inode_length(inode)) == sizeof e) {
            printf("write fail 1 - directory.c\n\n");
            ASSERT(false);
        }

        e.inode_sector = inode_get_inumber(dir->inode);
        e.name[0] = '.'; e.name[1] = '.'; e.name[2] = '0';
        e.is_dir = true;
        e.in_use = true;
        if(!inode_write_at (inode, &e, sizeof e, inode_length(inode)) == sizeof e) {
            printf("write fail 1 - directory.c\n\n");
            ASSERT(false);
        }
        inode_close(inode);
    }
    return true;
}

bool dir_remove (struct dir * dir, disk_sector_t sec)
{
    struct dir_entry e;
    off_t ofs;

    ASSERT (dir != NULL);

    for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) {
        if(e.in_use && e.inode_sector == sec) {
            e.in_use = false;
            if (!inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) {
                ASSERT(false);
            }
            struct inode * inode = inode_open(e.inode_sector);
            inode_remove(inode);
            inode_close(inode);
            return true;
        }
    }
    return false;
}


/* Reads the next directory entry in DIR and stores the name in
   NAME.  Returns true if successful, false if the directory
   contains no more entries. */
    bool
dir_readdir (struct dir *dir, char name[NAME_MAX + 1])
{
    struct dir_entry e;

    while (inode_read_at (dir->inode, &e, sizeof e, dir->pos) == sizeof e) 
    {
        dir->pos += sizeof e;
        if (e.in_use)
        {
            strlcpy (name, e.name, NAME_MAX + 1);
            return true;
        } 
    }
    return false;
}

/* project 4
 * sys call  */

bool dir_mkdir(char* path)
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

    if(success && !inode_create (new_sector, 0))
        success = false;

    struct dir * dir;
    dir = dir_open(inode_open(upper_dir_sector));
    success = success & dir_add(dir, dir_name, new_sector, true);
    dir_close(dir);


    if (!success && new_sector != 0) 
        free_map_release (new_sector, 1);

    return success;
}

bool dir_chdir(char* path)
{
    if(!dir_is_valid(path))
        return false;
    if(!dir_is_path_exist(path) || !dir_is_dir(path))
        return false;
    struct dir *old_dir = thread_current()->cur_dir;
    dir_close(old_dir);
    disk_sector_t sector = dir_get_sector_from_path(path);
    thread_current()->cur_dir = dir_open(inode_open(sector));
    return true;
}

/* project 4
 * path handling */



bool dir_is_valid(char* path)
{
    char *copy = malloc(sizeof(strlen(path)+1));
    char *token, *ptr;
    strlcpy(copy, path, strlen(path)+1);

    token = strtok_r(copy, "/", &ptr);
    if(token == NULL)
        return true;
    
    while(true) {
        if(strlen(token) > NAME_MAX) {
            return false;
        }
        token = strtok_r(NULL, "/", &ptr);
        if(token == NULL)
            break;
    }
    return true;
}

bool dir_is_path_exist(char* path)
{
    if(strlen(path) == 0) {
        // current
        return true;
    }

    struct dir * dir;
    if(path[0] == '/') {
        dir = dir_open_root();
        path = path+1;
    }
    else {
        dir = dir_reopen(thread_current()->cur_dir);
    }

    if(strlen(path) == 0)
        return true;


    struct dir_entry e;
    char *copy = malloc(sizeof(strlen(path)+1));
    char *token, *ptr;
    strlcpy(copy, path, strlen(path)+1);

    token = strtok_r(copy, "/", &ptr);
    while(true) {
        if(lookup(dir, token, &e, NULL, true)) {
            dir_close(dir);
            dir = dir_open(inode_open(e.inode_sector));
        }
        else if(lookup(dir, token, &e, NULL, false)) {
            dir_close(dir);
            token = strtok_r(NULL, "/", &ptr);
            if(token == NULL)
                return true;
            return false;
            
        } else {
            dir_close(dir);
            return false;
        }
        token = strtok_r(NULL, "/", &ptr);
        if(token == NULL)
            break;
    }
    return true;
}

bool dir_is_dir(char* path)
{
    struct dir * dir;
    if(path[0] == '/') {
        dir = dir_open_root();
        path = path+1;
    }
    else {
        dir = dir_reopen(thread_current()->cur_dir);
    }

    if(strlen(path) == 0)
        return true;

    struct dir_entry e;
    char *copy = malloc(sizeof(strlen(path)+1));
    char *token, *ptr;
    strlcpy(copy, path, strlen(path)+1);

    token = strtok_r(copy, "/", &ptr);
    while(true) {
        // dir
        if(lookup(dir, token, &e, NULL, true)) {
            dir_close(dir);
            dir = dir_open(inode_open(e.inode_sector));
        }
        else if(lookup(dir, token, &e, NULL, false)) {
            dir_close(dir);
            token = strtok_r(NULL, "/", &ptr);
            return false;
            
        } else {
            dir_close(dir);
            ASSERT(false);
        }
        token = strtok_r(NULL, "/", &ptr);
        if(token == NULL)
            break;
    }
    return true;

}

void dir_set_dir_name_from_path(char* dir_name, char* path)
{
    char *copy = malloc(sizeof(strlen(path)+1));
    strlcpy(copy, path, strlen(path)+1);

    char *token, *ptr, *last_path;
    token = strtok_r(copy, "/", &ptr);
    last_path = token;
    while(token) {
        token = strtok_r(NULL, "/", &ptr);
        if(token != NULL)
            last_path = token;
    }

    strlcpy(dir_name, last_path, strlen(last_path)+1);
    free(copy);

}

void dir_set_upper_path_from_path(char* dir_path, char* path)
{
    char dir_name[NAME_MAX+1];
    dir_set_dir_name_from_path(dir_name, path);
    int length = strlen(path) - strlen(dir_name);

    strlcpy(dir_path, path, length);
    dir_path[length] = 0;
}

disk_sector_t dir_get_sector_from_path(char *path)
{
    disk_sector_t sector;
    struct dir * dir;
    if(path[0] == '/') {
        dir = dir_open_root();
        path = path+1;
    }
    else {
        dir = dir_reopen(thread_current()->cur_dir);
    }

    if(strlen(path) == 0) {
        sector = inode_get_inumber(dir->inode);
        dir_close(dir);
        return sector;
    }

    struct dir_entry e;
    char *copy = malloc(sizeof(strlen(path)+1));
    char *token, *ptr;
    strlcpy(copy, path, strlen(path)+1);

    token = strtok_r(copy, "/", &ptr);
    while(true) {
        // dir
        if(lookup(dir, token, &e, NULL, true)) {
            dir_close(dir);
            sector = e.inode_sector;
            dir = dir_open(inode_open(e.inode_sector));
        }
        else if(lookup(dir, token, &e, NULL, false)) {
            sector = e.inode_sector;
            dir_close(dir);
            token = strtok_r(NULL, "/", &ptr);
            break;
            
        } else {
            dir_close(dir);
            ASSERT(false);
        }
        token = strtok_r(NULL, "/", &ptr);
        if(token == NULL)
            break;
    }
    return sector;
}

disk_sector_t dir_get_upper_sector_from_sector(disk_sector_t sec)
{
    struct dir * dir = dir_open(inode_open(sec));
    struct dir_entry e;
    char name[5];
    name[0] = '.'; name[1] = '.'; name[2] = 0;
    if (lookup (dir, name, &e, NULL, true)) {
        return e.inode_sector;
    }
    else {
        return -1;
    }
}

