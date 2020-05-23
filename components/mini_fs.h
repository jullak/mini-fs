#ifndef MINI_FS_MINI_FS_H
#define MINI_FS_MINI_FS_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bitmaps.h"
#include "direntry.h"
#include "fs_constants.h"
#include "inode.h"
#include "path.h"


#define ROOT_INODE 0

/*----------s u p e r b l o c k----------*/

typedef struct superblock {
  char magic[8];
  uint32_t inode_count;
  uint32_t block_count;
  uint32_t inode_size;
  uint32_t block_size;
} superblock_t;

superblock_t superblock_init(); // doesn't need destroy
bool superblock_compare(superblock_t * l, superblock_t * r);


/*----------m j f s  a p i----------*/

typedef struct mjfs_fs {
  FILE * fs_file;
  char * fs_file_name;

  inode_map_t inode_map;
  block_map_t block_map;

  inode_t current_directory;
  char * current_directory_name;
} mjfs_fs_t;

mjfs_fs_t init_dummy_fs();

int fs_init(char * fs_file); // public // EFILE

int fs_mount(mjfs_fs_t * fs, char * fs_file); // public // need to be free by unmount // EFILE
void fs_unmount(mjfs_fs_t * fs); // public

bool fs_is_root_init(mjfs_fs_t * fs); // private
int fs_init_root(mjfs_fs_t * fs); // private

int fs_init_new_inode(mjfs_fs_t * fs, inode_t * new_inode); // private // ENOSPACE
int fs_lock_inode(mjfs_fs_t * fs, int32_t inode_id); // private // ELOCK
void fs_free_inode(mjfs_fs_t * fs, int32_t inode_id); // private
int fs_lock_block(mjfs_fs_t * fs, int32_t block_id); // private // ELOCK
void fs_free_block(mjfs_fs_t * fs, int32_t block_id); // private

int fs_load_inode_by_path(mjfs_fs_t * fs, const char * path, inode_t * inode); // private // ENOTFOUND
int fs_load_inode(mjfs_fs_t * fs, int32_t inode_id, inode_t * inode); // private // ENOINODE
int fs_dump_inode(mjfs_fs_t * fs, int32_t inode_id, inode_t * inode); // private // ENOINODE

// return inode_id
int32_t fs_find_inode_by_path(mjfs_fs_t * fs, const char * path); // private // ENOTFOUND
int32_t fs_find_recursive(mjfs_fs_t * fs, inode_t * inode, path_t * path, int path_part); // private // ENOTFOUND
int32_t fs_find_inode_in_dir(mjfs_fs_t * fs, inode_t * dir, const char * name); // private // ENOTFOUND or ENOTADIR

int fs_delete_direntry_by_name(mjfs_fs_t * fs, inode_t * dir, const char * name); // private // ENOTFOUND
int fs_delete_file_inode(mjfs_fs_t * fs, inode_t * deleted); // private
int fs_delete_from_dir_by_name(mjfs_fs_t * fs, inode_t * dir, const char * name, bool recursive); // private // ENOTFOUND or ENOTADIR

int fs_write(mjfs_fs_t * fs, inode_t * inode, void * buf, uint32_t buf_len,
             uint32_t offset); // private // ENOSPACE or written bytes
int fs_write_to_dir(mjfs_fs_t * fs, inode_t * dir, direntry_t direntry); // private // ENOSPACE or 0

int fs_read(mjfs_fs_t * fs, inode_t * inode, void * dest, uint32_t dest_len,
        uint32_t offset); // private // EOFILE or read_bytes
int fs_read_from_dir(mjfs_fs_t * fs, inode_t * dir, uint32_t offset, direntry_t * direntry); // private // EEOFILE

int fs_reload_current_dir(mjfs_fs_t * fs); // private

//A P I
int fs_lookup_directory(mjfs_fs_t * fs, const char * directory, FILE * f); // public // ENOTFOUND
int fs_set_current_directory(mjfs_fs_t * fs, const char * directory); // public // ENOTFOUND
char * fs_pwd(mjfs_fs_t * fs); // does not to be free // could be NULL

int fs_create_directory(mjfs_fs_t * fs, const char * directory); // public // ENOSPACE or ENOTFOUND
int fs_create_file(mjfs_fs_t * fs, const char * file_name); // public // ENOSPACE or ENOTFOUND

int fs_delete_directory(mjfs_fs_t * fs, const char * directory); // public // ENOTFOUND or ENOTADIR
int fs_delete_file(mjfs_fs_t * fs, const char * file); // public // ENOTFOUND or ENOTADIR

int fs_load_to(mjfs_fs_t * fs, const char * external, const char * file);
int fs_store_from(mjfs_fs_t * fs, const char * file, const char * external);

int fs_cat(mjfs_fs_t * fs, const char * file, FILE * f);

int fs_mv(mjfs_fs_t * fs, const char * file, const char * destination);

/*----------d i r  e n t r y---------*/
typedef struct direntry_iter {
  uint32_t read_offset;
  uint32_t direntry_offset;
  direntry_t direntry;
  direntry_t * next_direntry;
  inode_t dir_inode;
} direntry_iter_t;

direntry_iter_t direntry_iter_init(mjfs_fs_t * fs, inode_t * dir_inode);
int direntry_next(mjfs_fs_t * fs, direntry_iter_t * direntry_iter);
void direntry_iter_destroy(direntry_iter_t * direntry_iter);


#endif //MINI_FS_MINI_FS_H