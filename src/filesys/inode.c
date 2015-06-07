#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

#define DIRECT_COUNT 10
#define SINGLE_INDIRECT_COUNT 10
#define UNUSED (128 - 1 - DIRECT_COUNT - SINGLE_INDIRECT_COUNT - 2)
#define INDIRECT_COUNT 128
#define NOT_ALLOCATED 9999

static char zeros[DISK_SECTOR_SIZE];

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
{
    off_t length;

    disk_sector_t direct_list[DIRECT_COUNT];
    disk_sector_t single_list[SINGLE_INDIRECT_COUNT];
    disk_sector_t double_elem;

    unsigned magic;                     /* Magic number. */
    uint32_t unused[UNUSED];               /* Not used. */
};

struct single_disk
{
    disk_sector_t direct_list[INDIRECT_COUNT];
};

struct double_disk
{
    disk_sector_t single_list[INDIRECT_COUNT];
};

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
    return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

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


/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
    void
inode_init (void) 
{
    list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   disk.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
    bool
inode_create (disk_sector_t sector, off_t length)
{
    ASSERT (length >= 0);

    /* If this assertion fails, the inode structure is not exactly
       one sector in size, and you should fix that. */
    ASSERT (sizeof(struct inode_disk)  == DISK_SECTOR_SIZE);
    ASSERT (sizeof(struct single_disk)  == DISK_SECTOR_SIZE);
    ASSERT (sizeof(struct double_disk)  == DISK_SECTOR_SIZE);

    bool success = false;

    struct inode_disk *disk_inode = calloc (1, sizeof *disk_inode);
    disk_inode->length = -1;
    disk_inode->magic = INODE_MAGIC;
    int i;
    for(i=0; i<DIRECT_COUNT; i++) {
        disk_inode->direct_list[i] = NOT_ALLOCATED;
    }
    for(i=0; i<SINGLE_INDIRECT_COUNT; i++) {
        disk_inode->single_list[i] = NOT_ALLOCATED;
    }
    disk_inode->double_elem = NOT_ALLOCATED;

    extend_file_if_necessary(disk_inode, length);

    cache_write(sector, disk_inode);
    free (disk_inode);
    success = true; 
    return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
    struct inode *
inode_open (disk_sector_t sector) 
{
    struct list_elem *e;
    struct inode *inode;

    /* Check whether this inode is already open. */
    for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
            e = list_next (e)) 
    {
        inode = list_entry (e, struct inode, elem);
        if (inode->sector == sector) 
        {
            inode_reopen (inode);
            return inode; 
        }
    }

    /* Allocate memory. */
    inode = malloc (sizeof *inode);
    if (inode == NULL)
        return NULL;

    /* Initialize. */
    list_push_front (&open_inodes, &inode->elem);
    inode->sector = sector;
    inode->open_cnt = 1;
    inode->deny_write_cnt = 0;
    inode->removed = false;
    cache_read (inode->sector, &inode->data);
    return inode;
}

/* Reopens and returns INODE. */
    struct inode *
inode_reopen (struct inode *inode)
{
    if (inode != NULL)
        inode->open_cnt++;
    return inode;
}

/* Returns INODE's inode number. */
    disk_sector_t
inode_get_inumber (const struct inode *inode)
{
    return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
    void
inode_close (struct inode *inode) 
{
    /* Ignore null pointer. */
    if (inode == NULL)
        return;

    /* Release resources if this was the last opener. */
    if (--inode->open_cnt == 0)
    {
        /* Remove from inode list and release lock. */
        list_remove (&inode->elem);

        /* Deallocate blocks if removed. */
        if (inode->removed) 
        {
            free_map_release (inode->sector, 1);
            // close later
            
            //free_map_release (inode->data.start,
            //        bytes_to_sectors (inode->data.length)); 
        }

        free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
    void
inode_remove (struct inode *inode) 
{
    ASSERT (inode != NULL);
    inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
    off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
    uint8_t *buffer = buffer_;
    off_t bytes_read = 0;
    uint8_t *bounce = NULL;

    if(size == 0)
        return 0;

    if(offset + size - 1 > inode_length(inode)) {
        ASSERT(false);
    }

    while (size > 0) 
    {
        int sector_ofs = offset % DISK_SECTOR_SIZE;
        int sector_left = DISK_SECTOR_SIZE - sector_ofs;

        off_t inode_left = inode_length (inode) - offset;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        int chunk_size = size < min_left ? size : min_left;
        if (chunk_size <= 0)
            break;

        disk_sector_t sector_idx = get_sector(&inode->data, offset / DISK_SECTOR_SIZE);

        if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) 
        {
            cache_read (sector_idx, buffer + bytes_read); 
        }
        else 
        {
            if (bounce == NULL) 
            {
                bounce = malloc (DISK_SECTOR_SIZE);
                if (bounce == NULL)
                    break;
            }
            cache_read (sector_idx, bounce);
            memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }

        size -= chunk_size;
        offset += chunk_size;
        bytes_read += chunk_size;
    }
    free (bounce);

    return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
    off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size, off_t offset) 
{
    const uint8_t *buffer = buffer_;
    off_t bytes_written = 0;
    uint8_t *bounce = NULL;

    if (inode->deny_write_cnt)
        return 0;

    if(size == 0)
        return 0;

    extend_file_if_necessary(&inode->data, offset + size - 1);

    while (size > 0) {
        int sector_ofs = offset % DISK_SECTOR_SIZE;
        int sector_left = DISK_SECTOR_SIZE - sector_ofs;

        off_t inode_left = inode_length (inode) - offset;
        int min_left = inode_left < sector_left ? inode_left : sector_left;

        int chunk_size = size < min_left ? size : min_left;
        if (chunk_size <= 0)
            break;

        disk_sector_t sector_idx = get_sector(&inode->data, offset / DISK_SECTOR_SIZE);

        if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) {

            cache_write (sector_idx, buffer + bytes_written); 

        } else {

            if (bounce == NULL) {
                bounce = malloc (DISK_SECTOR_SIZE);
                if (bounce == NULL)
                    break;
            }

            if (sector_ofs > 0 || chunk_size < sector_left) 
                cache_read (sector_idx, bounce);
            else
                memset (bounce, 0, DISK_SECTOR_SIZE);
            memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
            cache_write (sector_idx, bounce); 
        }

        size -= chunk_size;
        offset += chunk_size;
        bytes_written += chunk_size;
    }
    free (bounce);

    return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
    void
inode_deny_write (struct inode *inode) 
{
    inode->deny_write_cnt++;
    ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
    void
inode_allow_write (struct inode *inode) 
{
    ASSERT (inode->deny_write_cnt > 0);
    ASSERT (inode->deny_write_cnt <= inode->open_cnt);
    inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
    off_t
inode_length (const struct inode *inode)
{
    return inode->data.length;
}

disk_sector_t get_sector(struct inode_disk *disk_inode, off_t ith_sector)
{
    if(ith_sector < DIRECT_COUNT) {
        return disk_inode->direct_list[ith_sector];
    }

    ith_sector -= DIRECT_COUNT;
    
    if(ith_sector < SINGLE_INDIRECT_COUNT * INDIRECT_COUNT) {
        int i,j;
        i = ith_sector/INDIRECT_COUNT;
        j = ith_sector%INDIRECT_COUNT;
        if(disk_inode->single_list[i] == NOT_ALLOCATED) {
            ASSERT(false);
        }
        struct single_disk disk_single;
        cache_read(disk_inode->single_list[i], &disk_single);
        return disk_single.direct_list[j];
    }

    ith_sector -= SINGLE_INDIRECT_COUNT * INDIRECT_COUNT;
    
    if(ith_sector < INDIRECT_COUNT * INDIRECT_COUNT) {
        if(disk_inode->double_elem == NOT_ALLOCATED) {
            ASSERT(false);
        }
        struct double_disk disk_double;
        cache_read(disk_inode->double_elem, &disk_double);

        int i,j;
        i = ith_sector/INDIRECT_COUNT;
        j = ith_sector%INDIRECT_COUNT;

        if(disk_double.single_list[i] == NOT_ALLOCATED) {
            ASSERT(false);
        }
        struct single_disk disk_single;
        cache_read(disk_double.single_list[i], &disk_single);
        return disk_single.direct_list[j];
    }

    ASSERT(false);
}


void extend_file_if_necessary(struct inode_disk *disk_inode, off_t length)
{
    off_t origin_sectors = disk_inode->length / DISK_SECTOR_SIZE;
    off_t new_sectors = length / DISK_SECTOR_SIZE;
    
    if(origin_sectors >= new_sectors) {
        return;
    }

    uint8_t *bounce = malloc (DISK_SECTOR_SIZE);
    uint8_t *bounce2 = malloc (DISK_SECTOR_SIZE);

    off_t sectors = new_sectors - origin_sectors;
    
    // alloc new sectors from origin_sectors+1 ~ new_sectors
    // origin+sectors+1, +2, +3, ... + sectors

    int i_start, j_start;
    int i, j;


    i_start = origin_sectors+1;
    for(i=i_start; i<DIRECT_COUNT && sectors != 0; i++) {
        sectors--;
        disk_inode->direct_list[i] = alloc_sector();
        origin_sectors++;
    }

    origin_sectors -= DIRECT_COUNT;
    j_start = (origin_sectors+1) / INDIRECT_COUNT;
    for(j=j_start; j<SINGLE_INDIRECT_COUNT && sectors != 0; j++) {
        struct single_disk *disk_single = single_read_or_create(disk_inode->single_list, j, bounce);

        i_start = (origin_sectors+1) % INDIRECT_COUNT;
        for(i=i_start; i<INDIRECT_COUNT && sectors != 0; i++) {
            sectors--;
            disk_single->direct_list[i] = alloc_sector();
            origin_sectors++;
        }

        single_writeback_or_alloc(disk_inode->single_list, j, disk_single);
    }

    origin_sectors -= SINGLE_INDIRECT_COUNT * INDIRECT_COUNT;
    if(sectors != 0) {
        struct double_disk *disk_double = double_read_or_create(disk_inode->double_elem, bounce);

        j_start = (origin_sectors+1)/INDIRECT_COUNT;
        for(j=j_start; j<INDIRECT_COUNT && sectors != 0; j++) {
            struct single_disk *disk_single = single_read_or_create(disk_double->single_list, j, bounce2);

            i_start = (origin_sectors+1)%INDIRECT_COUNT;
            for(i=i_start; i<INDIRECT_COUNT && sectors != 0; i++) {
                sectors--;
                disk_single->direct_list[i] = alloc_sector();
                origin_sectors++;
            }

            single_writeback_or_alloc(disk_double->single_list, j, disk_single);
        }

        double_writeback_or_alloc(disk_inode->double_elem, disk_double);
    }

    free(bounce);

    disk_inode->length = length;
}


struct single_disk * single_read_or_create(disk_sector_t list[], int j, uint8_t *bounce)
{
    if(list[j] == NOT_ALLOCATED) {
        return calloc(1, sizeof(struct single_disk));
    }
    else {
        cache_read(list[j], bounce);
        return (struct single_disk *)bounce;
    }
    ASSERT(false);
}

void single_writeback_or_alloc(disk_sector_t list[], int j, struct single_disk * disk_single)
{
    if(list[j] == NOT_ALLOCATED) {
        list[j] = alloc_single(disk_single);
        free(disk_single);
    }
    else {
        cache_write(list[j], disk_single);
    }
}


struct double_disk * double_read_or_create(disk_sector_t double_elem, uint8_t *bounce)
{
    if(double_elem == NOT_ALLOCATED) {
        struct double_disk * d = calloc(1, sizeof(struct double_disk));
        int i;
        for(i=0; i<INDIRECT_COUNT; i++) {
            d->single_list[i] = NOT_ALLOCATED;
        }
        return d;
    }
    else {
        cache_read(double_elem, bounce);
        return (struct double_disk *)bounce;
    }
    ASSERT(false);
}

void double_writeback_or_alloc(disk_sector_t double_elem, struct double_disk * disk_double) {
    if(double_elem == NOT_ALLOCATED) {
        double_elem = alloc_double(disk_double);
        free(disk_double);
    }
    else {
        cache_write(double_elem, disk_double);
    }
}

disk_sector_t alloc_sector()
{
    disk_sector_t sec_no;
    free_map_allocate(1, &sec_no);
    cache_write(sec_no, zeros);
    return sec_no;
}

disk_sector_t alloc_single(struct single_disk *disk_single)
{
    disk_sector_t sec_no;
    free_map_allocate(1, &sec_no);
    cache_write(sec_no, disk_single);
    return sec_no;
}

disk_sector_t alloc_double(struct double_disk *disk_double)
{
    disk_sector_t sec_no;
    free_map_allocate(1, &sec_no);
    cache_write(sec_no, disk_double);
    return sec_no;
}
