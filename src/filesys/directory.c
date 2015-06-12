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

//    printf("-----------------list--------------------\n");
    for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e;
            ofs += sizeof e)  {
 //       if(e.in_use) {
 //           printf("%s\n", e.name);
 //       }
        if (e.is_dir == is_dir && e.in_use && !strcmp (name, e.name))
        {
            if (ep != NULL)
                *ep = e;
            if (ofsp != NULL)
                *ofsp = ofs;
            return true;
        }
    }
 //   printf("---------------done-----------------\n");
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

    struct inode * inode = inode_open(inode_sector);
    if(inode == NULL) {
        return false;
    }
    /* Write slot. */
    e.inode_sector = inode_sector;
    strlcpy (e.name, name, sizeof e.name);
    e.is_dir = is_dir;
    e.in_use = true;
    if(inode_write_at (dir->inode, &e, sizeof e, ofs) != sizeof e) {
        inode_close(inode);

        return false;
    }

    if(is_dir) {

        e.inode_sector = inode_sector;
        e.name[0] = '.'; e.name[1] = 0;
        e.is_dir = true;
        e.in_use = true;
        if(inode_write_at (inode, &e, sizeof e, inode_length(inode)) != sizeof e) {
            printf("write fail 1 - directory.c\n\n");
            ASSERT(false);
        }

        e.inode_sector = inode_get_inumber(dir->inode);
        e.name[0] = '.'; e.name[1] = '.'; e.name[2] = 0;
        e.is_dir = true;
        e.in_use = true;
        if(inode_write_at (inode, &e, sizeof e, inode_length(inode)) != sizeof e) {
            printf("write fail 1 - directory.c\n\n");
            ASSERT(false);
        }
    }
    inode_close(inode);
    return true;
}

bool dir_remove (struct dir * dir, disk_sector_t sec)
{
    struct dir_entry e;
    off_t ofs;

    ASSERT (dir != NULL);

    for (ofs = 0; inode_read_at (dir->inode, &e, sizeof e, ofs) == sizeof e; ofs += sizeof e) {
        if(e.in_use && e.inode_sector == sec) {
            struct dir_entry en;
            en.in_use = false;

            if (inode_write_at (dir->inode, &en, sizeof en, ofs) != sizeof en) {
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

/* project 4
 * sys call  */

bool dir_mkdir(char* path)
{
    if(strlen(path) == 0)
        return false;
    disk_sector_t new_sector = 0;
    disk_sector_t upper_dir_sector = 0;

    if(!dir_is_valid(path))
        return false;

    char dir_name[NAME_MAX+1];
    char *upper_dir_path = malloc(sizeof(strlen(path)+1));

    dir_set_dir_name_from_path(dir_name, path);
    dir_set_upper_path_from_path(upper_dir_path, path);

    if(!dir_is_path_exist(upper_dir_path) || !dir_is_dir(upper_dir_path)) {
        free(upper_dir_path);
        return false;
    }

    upper_dir_sector = dir_get_sector_from_path(upper_dir_path);
    free(upper_dir_path);



    if(!free_map_allocate (1, &new_sector)) {
        return false;
    }

    if(!inode_create (new_sector, 0)) {
        free_map_release(new_sector, 1);
        return false;
    }

    struct dir * dir;
    bool success;
    dir = dir_open(inode_open(upper_dir_sector));
    success = dir_add(dir, dir_name, new_sector, true);
    dir_close(dir);

    if(success == false) {
        free_map_release (new_sector, 1);
        return false;
    }
    return true;
}

bool dir_chdir(char* path)
{
    if(strlen(path) == 0)
        return false;
    if(!dir_is_valid(path))
        return false;

    if(!dir_is_path_exist(path) || !dir_is_dir(path))
        return false;
    struct dir *old_dir = thread_current()->cur_dir;
    disk_sector_t sector = dir_get_sector_from_path(path);
    thread_current()->cur_dir = dir_open(inode_open(sector));
    dir_close(old_dir);
    return true;
}

bool dir_readdir(struct file * file, char* name)
{
    struct dir_entry e;

    while (inode_read_at (file->inode, &e, sizeof e, file->pos) == sizeof e) 
    {
        file->pos += sizeof e;
        if (e.in_use && strcmp(e.name, "..") && strcmp(e.name, "."))
        {
            strlcpy (name, e.name, NAME_MAX + 1);
            return true;
        } 
    }
    return false;

}

/* project 4
 * path handling */



bool dir_is_valid(char* path)
{
    char *copy = malloc(sizeof(strlen(path)+1));
    char *token, *ptr;
    strlcpy(copy, path, strlen(path)+1);

    token = strtok_r(copy, "/", &ptr);
    if(token == NULL) {
        free(copy);
        return true;
    }
    
    while(true) {
        if(strlen(token) > NAME_MAX) {
            free(copy);
            return false;
        }
        token = strtok_r(NULL, "/", &ptr);
        if(token == NULL)
            break;
    }
    free(copy);
    return true;
}

bool dir_is_path_exist(char* path)
{

    struct dir * dir;
    if(path[0] == '/') {
        dir = dir_open_root();
        path = path+1;
    }
    else {
        dir = dir_reopen(thread_current()->cur_dir);
    }

    if(dir->inode->removed)
        return false;

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
            if(dir->inode->removed) {
                dir_close(dir);
                free(copy);
                return false;
            }
        }
        else if(lookup(dir, token, &e, NULL, false)) {
            dir_close(dir);
            struct inode * inode = inode_open(e.inode_sector);
            if(inode->removed) {
                inode_close(inode);
                free(copy);
                return false;
            }
            inode_close(inode);

            token = strtok_r(NULL, "/", &ptr);
            free(copy);
            if(token == NULL)
                return true;
            return false;
            
        } else {
            dir_close(dir);
            free(copy);
            return false;
        }
        token = strtok_r(NULL, "/", &ptr);
        if(token == NULL) {
            dir_close(dir);
            break;
        }
    }
    free(copy);
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
            free(copy);
            return false;
            
        } else {
            dir_close(dir);
            ASSERT(false);
        }
        token = strtok_r(NULL, "/", &ptr);
        if(token == NULL) {
            dir_close(dir);
            break;
        }
    }
    free(copy);
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
        if(token == NULL) {
            dir_close(dir);
            break;
        }
    }
    free(copy);
    return sector;
}

disk_sector_t dir_get_upper_sector_from_sector(disk_sector_t sec)
{
    struct dir * dir = dir_open(inode_open(sec));
    struct dir_entry e;
    char name[5];
    name[0] = '.'; name[1] = '.'; name[2] = 0;
    if (lookup (dir, name, &e, NULL, true)) {
        dir_close(dir);
        return e.inode_sector;
    }
    else {
        dir_close(dir);
        return -1;
    }
}

