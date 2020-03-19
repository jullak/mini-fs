#include "mini_fs.h"

#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

path_t path_init(char * path) {
    path_t new_path;
    int occurrence = 0;
    for (int ptr = 0; path[ptr] != '\0'; ++ptr, (path[ptr] == '/') ? ++occurrence : occurrence);

    new_path.full_path = calloc(strlen(path), 1);
    new_path.tokens = calloc(occurrence, sizeof(char *));

    strcpy(new_path.full_path, path);
    int ptr = 0;
    new_path.tokens[ptr] = strtok(new_path.full_path, "/");
    while (new_path.tokens[ptr] != NULL) {
        ++ptr;
        new_path.tokens[ptr] = strtok(NULL, "/");
    }

    return new_path;
}
void path_destroy(path_t * path) {
    free(path->full_path);
    free(path->tokens);
}

/*----------i n o d e----------*/
int inode_load_block(inode_t * inode, size_t offset) {
    int block_id = offset / BLOCK_SIZE;
    if (offset >= inode->file_size || block_id >= inode->block_count) { //???
        return EREQBLOCK;
    }
    return block_id;
}
int inode_load_block_offset(inode_t * inode, size_t offset) {
    if (offset >= inode->file_size || offset / BLOCK_SIZE >= inode->block_count) {
        return EREQBLOCK;
    }
    return offset % BLOCK_SIZE;
}
int inode_add_block(inode_t * inode, int block_id) { //нет косвенной адресации
    if (inode->block_count >= 4) {//TODO: blocks constant!!
        return ENOSPACE;
    }
    inode->address[inode->block_count] = block_id;
    ++inode->block_count;
    return block_id;
}
void inode_update_timestamp(inode_t * inode) {
    time(&inode->timestamp);
}
char * inode_info_str(inode_t * inode) {
    char * info = "this is realization";
    return info;
}


/*----------m j f s  a p i----------*/
mjfs_fs_t init_dummy_fs() {
    mjfs_fs_t dummy;
    dummy.fs_file = NULL;
    dummy.fs_file_name = NULL;
    dummy.current_directory_name = NULL;
    return dummy;
}

int fs_init(char * fs_file) {
    int fd = creat(fs_file, 0700);
    if (fd == -1) {
        return EFILE;
    }
    /*if (-1 == truncate(fs_file, FS_SIZE)) {
        return EFILE;
    }*/

    superblock_t superblock = superblock_init();
    write(fd, &superblock, sizeof(superblock_t));

    inode_map_t inode_map;
    memset(&inode_map.bitmap, 0, INODE_COUNT/8);
    write(fd, &inode_map.bitmap, INODE_COUNT/8);

    block_map_t block_map;
    memset(&block_map.bitmap, 0, BLOCK_COUNT/8);
    write(fd, &block_map.bitmap, BLOCK_COUNT/8);

    close(fd);
    return 0;
}

int fs_mount(mjfs_fs_t * fs, char * fs_file) {
    fs->fs_file_name = calloc(strlen(fs_file), 1);
    strcpy(fs->fs_file_name, fs_file);

    fs->fs_file = fopen(fs_file, "rw");
    if (!fs->fs_file) {
        return EFILE;
    }

    superblock_t reference_block = superblock_init();
    superblock_t fs_block;

    fread(&fs_block, sizeof(char), sizeof(superblock_t), fs->fs_file);
    if (superblock_compare(&fs_block, &reference_block)) {
        return EFILE;
    }

    fread(fs->inode_map.bitmap, sizeof(char), sizeof(fs->inode_map.bitmap), fs->fs_file);
    fread(fs->block_map.bitmap, sizeof(char), sizeof(fs->block_map.bitmap), fs->fs_file);


    fs->current_directory_name = NULL;
    if (!fs_is_root_init(fs)) {
        fs_init_root(fs);
    }
    fs_load_inode(fs, &fs->current_directory, ROOT_INODE);
    //fs_lookup

    return 0;
}

void fs_unmount(mjfs_fs_t * fs) {
    fclose(fs->fs_file);
    free(fs->fs_file_name);
    free(fs->current_directory_name);
}


int fs_lock_inode(mjfs_fs_t * fs, int inode_id) {
    if (ELOCK == bitmap_set_lock(&fs->inode_map.bitmap, inode_id)) {
        return ELOCK;
    }
    fseek(fs->fs_file, sizeof(superblock_t), SEEK_SET);
    fwrite(&fs->inode_map.bitmap, sizeof(char), INODE_COUNT/8, fs->fs_file);
    return 0;
}
void fs_free_inode(mjfs_fs_t * fs, int inode_id) {
    bitmap_set_free(&fs->inode_map.bitmap, inode_id);
    fseek(fs->fs_file, sizeof(superblock_t), SEEK_SET);
    fwrite(&fs->inode_map.bitmap, sizeof(char), INODE_COUNT/8, fs->fs_file);
}
int fs_lock_block(mjfs_fs_t * fs, int block_id) {
    if (ELOCK == bitmap_set_lock(&fs->block_map.bitmap, block_id)) {
        return ELOCK;
    }
    fseek(fs->fs_file, sizeof(superblock_t) + INODE_COUNT/8, SEEK_SET);
    fwrite(&fs->block_map.bitmap, sizeof(char), BLOCK_COUNT/8, fs->fs_file);
    return 0;
}
void fs_free_block(mjfs_fs_t * fs, int block_id) {
    bitmap_set_free(&fs->block_map.bitmap, block_id);
    fseek(fs->fs_file, sizeof(superblock_t) + INODE_COUNT/8, SEEK_SET);
    fwrite(&fs->block_map.bitmap, sizeof(char), BLOCK_COUNT/8, fs->fs_file);
}

bool fs_is_root_init(mjfs_fs_t * fs) {
    return bitmap_is_lock(&fs->inode_map, ROOT_INODE);
}
int fs_init_root(mjfs_fs_t * fs) {
    inode_t root;
    bitmap_set_lock(&fs->inode_map.bitmap, ROOT_INODE);
    fs_load_inode(fs, ROOT_INODE, &root);

    root.id = ROOT_INODE;

    //TODO write two names

    fs_dump_inode(fs, &root);
    return 0;
}
int fs_load_inode(mjfs_fs_t * fs, int inode_id, inode_t * inode) {
    int load_from = INODE_SECTION + inode_id * INODE_SIZE;

    if (load_from >= BLOCK_SECTION) {
        return ENOINODE;
    }

    fseek(fs->fs_file, load_from, SEEK_SET);
    fread(inode, sizeof(inode_t), 1, fs->fs_file);
    return 0;
}
int fs_dump_inode(mjfs_fs_t * fs, inode_t * inode) {
    int load_from = INODE_SECTION + inode->id * INODE_SIZE;

    if (load_from >= BLOCK_SECTION) {
        return ENOINODE;
    }

    fseek(fs->fs_file, load_from, SEEK_SET);
    fwrite(inode, sizeof(inode_t), 1, fs->fs_file);
    return 0;
}
int fs_find_inode(mjfs_fs_t * fs, char * path) {
    path_t split_path = path_init(path);
    if (path[0] == "/") {
        fs_recursive_search(fs, &fs->current_directory, &split_path, 0);
    } else {
        inode_t root;
        if (ENOINODE == fs_load_inode(fs, ROOT_INODE, &root)) {
            return ENOTFOUND;
        }
        return fs_recursive_search(fs, &root, &split_path, 0);
    }

    path_destroy(&split_path);
    return ENOTFOUND;
}
int fs_recursive_search(mjfs_fs_t * fs, inode_t * inode, path_t * path, int path_part) {
    int inode_id = find_in_dir(fs, inode, path->tokens[path_part]);
    if (ENOTFOUND == inode_id) {
        return ENOTFOUND;
    }
    if (path->tokens[path_part + 1] == NULL) {
        return inode_id;
    }

    inode_t current_inode;
    if (ENOINODE == fs_load_inode(fs, inode_id, &current_inode)) {
        return ENOTFOUND;
    }
    return fs_recursive_search(fs, &current_inode, path, path_part + 1);
}
int find_in_dir(mjfs_fs_t * fs, inode_t * inode, char * path) {
    int seek_value = 0;

    int inode_id = 0;
    int name_size = 0;
    char * name;

    while(seek_value < fs->current_directory.file_size) {
        fs_read(fs, &fs->current_directory, &name_size, sizeof(int), seek_value);
        name = malloc(name_size);
        seek_value += sizeof(int);
        fs_read(fs, &fs->current_directory, name, sizeof(char), seek_value);
        seek_value += name_size;

        if (strcmp(name, path)) {
            fs_read(fs, &fs->current_directory, &inode_id, sizeof(int), seek_value);

            free(name);
            return inode_id;
        }

        seek_value += sizeof(int);
        free(name);
    }

    return ENOTFOUND;
}


//тут конечно же написана хуйня
//upd уже лучше
int fs_write(mjfs_fs_t * fs, inode_t * inode, void * buf, size_t buf_len, size_t offset) {
    int block_id;
    int block_offset;
    int written_bytes = 0;
    int current_offset = offset;

    while (written_bytes < buf_len) {
        block_id = inode_load_block(inode, current_offset);
        if (block_id == EREQBLOCK) {
            int free_block = bitmap_find_free(&fs->block_map.bitmap);
            if (ENOSPACE == free_block) {
                return ENOSPACE;
            }

            fs_lock_block(fs, free_block);
            if (ENOSPACE == inode_add_block(inode, free_block)) {
                return ENOSPACE;
            }
            block_id = free_block;
        }
        block_offset = inode_load_block_offset(inode, current_offset);

        int write_to = BLOCK_SECTION + block_id * BLOCK_SIZE + block_offset;

        fseek(fs->fs_file, write_to, SEEK_SET);
        int need_write = (buf_len - written_bytes < BLOCK_SIZE - block_offset) ? (buf_len - written_bytes) : (BLOCK_SIZE - block_offset);
        fwrite(buf + written_bytes, sizeof(char), need_write, fs->fs_file);
        //upd size
        written_bytes += need_write;
        current_offset += written_bytes;
        inode->file_size += written_bytes;
    }

    inode_update_timestamp(inode);

    return 0;
}
int fs_write_path(mjfs_fs_t * fs, char * path, void * buf, size_t buf_len, size_t offset) {
    int inode_id = fs_find_inode(fs, path);
    if (ENOTFOUND == inode_id) {
        return ENOTFOUND;
    }
    inode_t inode_file;
    fs_load_inode(fs, inode_id, &inode_file);

    return fs_write(fs, &inode_file, buf, buf_len, offset);
}


int fs_read(mjfs_fs_t * fs, inode_t * inode, void * dest, size_t dest_len, size_t offset) {
    int block_id;
    int block_offset;

    int read_bytes = 0;
    int current_offset = offset;

    while (read_bytes < dest_len) {
        block_id = inode_load_block(inode, current_offset);
        if (block_id == EREQBLOCK) {
            return EEOFILE;
        }
        block_offset = inode_load_block_offset(inode, current_offset);

        int read_from = BLOCK_SECTION + block_id * BLOCK_SIZE + block_offset;
        fseek(fs->fs_file, read_from, SEEK_SET);
        int need_write = (dest_len - read_bytes < BLOCK_SIZE - block_offset) ? (dest_len - read_bytes) : (BLOCK_SIZE - block_offset);
        fread(dest + read_bytes, sizeof(char), need_write, fs->fs_file);

        read_bytes += need_write;
        current_offset += read_bytes;
    }
}


int fs_create_directory(mjfs_fs_t * fs, char * directory) {
    int inode_id = bitmap_find_free(fs->inode_map.bitmap);
    if (inode_id == ENOSPACE) {
        return ENOSPACE;
    }

    if (ELOCK == fs_lock_inode(fs, inode_id)) {
        return ELOCK;
    }

    inode_t inode;
    inode.id = inode_id;

    //fs_write_default_dir_info(fs, &inode);

    fs_dump_inode(fs, &inode);

    return 0;
}
int fs_set_current_directory(mjfs_fs_t * fs, char * directory) {
    int inode_id = fs_find_inode(fs, directory);
    if (ENOTFOUND == inode_id) {
        return ENOTFOUND;
    }
    fs_load_inode(fs, inode_id, &fs->current_directory);

    if (directory[0] == "/") {
        free(fs->current_directory_name);
        fs->current_directory_name = calloc(sizeof(directory), 1);
        strcpy(fs->current_directory_name, directory);
    } else {
        char * new_directory_path = calloc(sizeof(directory) + sizeof(fs->current_directory_name) + 1, 1);
        strcat(new_directory_path, fs->current_directory_name);
        strcat(new_directory_path, "/");
        strcat(new_directory_path, directory);

        free(fs->current_directory_name);
        fs->current_directory_name = new_directory_path;
    }
    return 0;
}
char * fs_pwd(mjfs_fs_t * fs) { // do not need free
    return fs->current_directory_name;
}


/*----------s u p e r b l o c k----------*/
superblock_t superblock_init() {
    superblock_t superblock;

    superblock.magic[0] = 0;
    superblock.magic[1] = 25;
    superblock.magic[2] = 52;
    superblock.magic[3] = 0;

    superblock.magic[4] = 'M';
    superblock.magic[5] = 'J';
    superblock.magic[6] = 'F';
    superblock.magic[7] = 'S';

    superblock.inode_count = INODE_COUNT;
    superblock.block_count = BLOCK_COUNT;
    superblock.inode_size = INODE_SIZE;
    superblock.block_size = BLOCK_SIZE;

    superblock.root_inode = ROOT_INODE;

    return superblock;
}
bool superblock_compare(superblock_t * l, superblock_t * r) {
    bool is_equal = true;

    if (0 != strcmp(l->magic, r->magic)
        || l->inode_count != r->inode_count
        || l->block_count != r->block_count
        || l->inode_size != r->inode_size
        || l->block_size != r->block_size) {
        is_equal = false;
    }

    return is_equal;
}




/*----------b i t m a p----------*/
int bitmap_find_free(char * bitmap) {
    for (int i = 0; i < sizeof(bitmap) / sizeof(char); ++i) {
        for (int shift = 8; shift >= 0; --shift) {
            if (bitmap[i] & (1 << shift))
            {
                return i * 8 + (8 - shift);
            }
        }
    }
    return ENOSPACE;
}
int bitmap_set_lock(char * bitmap[], int id) {
    int bitmap_id = id / 8;
    int pos = id % 8;

    if ((*bitmap)[bitmap_id] & (1 << pos)){
        return ELOCK;
    }

    (*bitmap)[bitmap_id] |= (1 << pos);
    return 0;
}
int bitmap_set_free(char * bitmap[], int id) {
    int bitmap_id = id / 8;
    int pos = id % 8;

    (*bitmap)[bitmap_id] &= ~(1 << pos);
    return 0;
}
bool bitmap_is_lock(char * bitmap[], int id) {
    int bitmap_id = id / 8;
    int pos = id % 8;

    return (*bitmap)[bitmap_id] & (1 << pos);
}