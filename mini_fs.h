#ifndef MINI_FS_MINI_FS_H
#define MINI_FS_MINI_FS_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

//byte
#define FS_SIZE 8192

//tmp
#define BLOCK_SIZE 128
#define INODE_SIZE 32
#define BLOCK_COUNT 16
#define INODE_COUNT 4

#define BLOCK_SECTION sizeof(superblock_t) + INODE_COUNT / 8 + BLOCK_COUNT / 8 + INODE_COUNT * INODE_SIZE
#define INODE_SECTION sizeof(superblock_t) + INODE_COUNT / 8 + BLOCK_COUNT / 8;

#define ROOT_INODE 0

enum error {
    EFILE = -1,
    ENOSPACE = -2,
    ELOCK = -3,
    EREQBLOCK = -4,
    ENOINODE = -5,
    ENOTFOUND = -6,
    EEOFILE = -9,
};

/*----------p a t h----------*/
typedef struct path {
    char * full_path;
    char ** tokens;
} path_t;
path_t path_init(char * path);


/*----------s u p e r b l o c k----------*/

typedef struct superblock {
    char magic[8];
    size_t inode_count;
    size_t block_count;
    size_t inode_size;
    size_t block_size;
    size_t root_inode;
} superblock_t;

superblock_t superblock_init();
bool superblock_compare(superblock_t * l, superblock_t * r);



/*----------b i t m a p----------*/

typedef struct inode_map {
    char bitmap[INODE_SIZE/8];
} inode_map_t;

typedef struct block_map {
    char bitmap[BLOCK_SIZE/8];
} block_map_t;

int bitmap_find_free(char * bitmap);
int bitmap_set_lock(char * bitmap[], int id);
int bitmap_set_free(char * bitmap[], int id);
bool bitmap_is_lock(char * bitmap[], int id);



/*----------i n o d e----------*/

typedef struct inode {
    size_t id;
    size_t file_size;
    size_t block_count;
    time_t timestamp;
    int address[4]; //TODO: warning! test value
} inode_t;

int inode_load_block(inode_t * inode, size_t offset);
int inode_load_block_offset(inode_t * inode, size_t offset);
int inode_add_block(inode_t * inode, int block_id);
void inode_update_timestamp(inode_t * inode);
char * inode_info_str(inode_t * inode);



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

int fs_init(char * fs_file); // public

int fs_mount(mjfs_fs_t * fs, char * fs_file); // public
void fs_unmount(mjfs_fs_t * fs); // public

int fs_lock_inode(mjfs_fs_t * fs, int inode_id); // private
void fs_free_inode(mjfs_fs_t * fs, int inode_id); // private
int fs_lock_block(mjfs_fs_t * fs, int block_id); // private
void fs_free_block(mjfs_fs_t * fs, int block_id); // private

bool fs_is_root_init(mjfs_fs_t * fs);
int fs_init_root(mjfs_fs_t * fs);
int fs_load_inode(mjfs_fs_t * fs, int inode_id, inode_t * inode); // private
int fs_dump_inode(mjfs_fs_t * fs, inode_t * inode); // private
int fs_find_inode(mjfs_fs_t *fs, char * path); // private return - inode_id
int fs_recursive_search(mjfs_fs_t * fs, inode_t * inode, path_t * path, int path_part); // private
int find_in_dir(mjfs_fs_t * fs, inode_t * inode, char * path); // private

int fs_write(mjfs_fs_t * fs, inode_t * inode, void * buf, size_t buf_len, size_t offset); // private
int fs_write_path(mjfs_fs_t * fs, char * path, void * buf, size_t buf_len, size_t offset); // public
int fs_write_to_dir(mjfs_fs_t * fs, inode_t * dir, char * name);
int fs_write_default_dir_info(mjfs_fs_t * fs, inode_t * dir);

int fs_read(mjfs_fs_t * fs, inode_t * inode, void * dest, size_t dest_len, size_t offset); // private

int fs_set_current_directory(mjfs_fs_t * fs, char * directory); // public
char * fs_pwd(mjfs_fs_t * fs);

int fs_create_directory(mjfs_fs_t * fs, char * directory); // public
int fs_create_file(); // public

int fs_delete_directory(); // public
int fs_delete_file(); // public

int fs_lookup_directory(); // public

//cp and mv!

int fs_cp_to_host(); // public
int fs_cp_from_host(); //public

#endif //MINI_FS_MINI_FS_H
