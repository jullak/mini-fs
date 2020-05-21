#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "errors.h"
#include "mini_fs.h"


/*----------m j f s  a p i----------*/
mjfs_fs_t init_dummy_fs() {
  mjfs_fs_t dummy;
  dummy.fs_file = NULL;
  dummy.fs_file_name = NULL;
  dummy.current_directory_name = NULL;
  return dummy;
}

int fs_init(char * fs_file) {
  int fd = creat(fs_file, 0777);
  if (fd == -1) {
    return EFILE;
  }
  if (-1 == truncate(fs_file, FS_SIZE)) {
      close(fd);
      return EFILE;
  }

  superblock_t superblock = superblock_init();
  write(fd, &superblock, sizeof(superblock_t));

  inode_map_t inode_map;
  memset(inode_map.bitmap, 0, INODE_COUNT / 8);
  write(fd, inode_map.bitmap, INODE_COUNT / 8);

  block_map_t block_map;
  memset(block_map.bitmap, 0, BLOCK_COUNT / 8);
  write(fd, block_map.bitmap, BLOCK_COUNT / 8);

  close(fd);
  return 0;
}

int fs_mount(mjfs_fs_t * fs, char * fs_file) {
  fs->fs_file_name = calloc(strlen(fs_file) + 1, 1);
  strcpy(fs->fs_file_name, fs_file);
  fs->current_directory_name = NULL;

  fs->fs_file = fopen(fs_file, "rb+");
  if (NULL == fs->fs_file) {
    fs_unmount(fs);
    return EFILE;
  }

  superblock_t reference_block = superblock_init();
  superblock_t fs_block;

  fread(&fs_block, sizeof(char), sizeof(superblock_t), fs->fs_file);
  if (!superblock_compare(&fs_block, &reference_block)) {
    fs_unmount(fs);
    return EFILE;
  }

  fread(fs->inode_map.bitmap, sizeof(char), sizeof(fs->inode_map.bitmap), fs->fs_file);
  fread(fs->block_map.bitmap, sizeof(char), sizeof(fs->block_map.bitmap), fs->fs_file);


  if (!fs_is_root_init(fs)) {
    if (ENOSPACE == fs_init_root(fs)) {
      fs_unmount(fs);
      return ENOSPACE;
    }
  }
  fs_load_inode(fs, ROOT_INODE, &fs->current_directory);
  fs->current_directory_name = calloc(1, 2);
  strcat(fs->current_directory_name, "/\0");

  return 0;
}

void fs_unmount(mjfs_fs_t * fs) {
  fclose(fs->fs_file);
  free(fs->fs_file_name);
  free(fs->current_directory_name);
}

bool fs_is_root_init(mjfs_fs_t * fs) {
  return bitmap_is_lock(fs->inode_map.bitmap, ROOT_INODE);
}

int fs_init_root(mjfs_fs_t * fs) {
  inode_t root;
  fs_lock_inode(fs, ROOT_INODE);

  fs_load_inode(fs, ROOT_INODE, &root);
  memset(&root, 0, sizeof(inode_t));
  root.id = ROOT_INODE;
  root.is_dir = true;

  if (ENOSPACE == fs_write_to_dir(fs, &root, direntry_init(ROOT_INODE, "."))) {
    return ENOSPACE;
  }
  if (ENOSPACE == fs_write_to_dir(fs, &root, direntry_init(ROOT_INODE, ".."))) {
    return ENOSPACE;
  }

  fs_dump_inode(fs, ROOT_INODE, &root);
  return 0;
}


int fs_init_new_inode(mjfs_fs_t * fs, inode_t * new_inode) {
  int32_t inode_id = bitmap_find_free(fs->inode_map.bitmap);
  if (ENOSPACE == inode_id) {
    return ENOSPACE;
  }

  if (ELOCK == fs_lock_inode(fs, inode_id)) {
    return ENOSPACE;
  }

  fs_load_inode(fs, inode_id, new_inode);
  memset(new_inode, 0, sizeof(inode_t));
  new_inode->id = inode_id;

  return 0;
}

int fs_lock_inode(mjfs_fs_t * fs, int32_t inode_id) {
  if (ELOCK == bitmap_set_lock(fs->inode_map.bitmap, inode_id)) {
    return ELOCK;
  }
  fseek(fs->fs_file, sizeof(superblock_t), SEEK_SET);
  fwrite(fs->inode_map.bitmap, sizeof(char), INODE_COUNT / 8, fs->fs_file);
  return 0;
}

void fs_free_inode(mjfs_fs_t * fs, int32_t inode_id) {
  bitmap_set_free(fs->inode_map.bitmap, inode_id);
  fseek(fs->fs_file, sizeof(superblock_t), SEEK_SET);
  fwrite(fs->inode_map.bitmap, sizeof(char), INODE_COUNT / 8, fs->fs_file);
}

int fs_lock_block(mjfs_fs_t * fs, int32_t block_id) {
  if (ELOCK == bitmap_set_lock(fs->block_map.bitmap, block_id)) {
    return ELOCK;
  }
  fseek(fs->fs_file, sizeof(superblock_t) + INODE_COUNT / 8, SEEK_SET);
  fwrite(fs->block_map.bitmap, sizeof(char), BLOCK_COUNT / 8, fs->fs_file);
  return 0;
}

void fs_free_block(mjfs_fs_t * fs, int32_t block_id) {
  bitmap_set_free(fs->block_map.bitmap, block_id);
  fseek(fs->fs_file, sizeof(superblock_t) + INODE_COUNT / 8, SEEK_SET);
  fwrite(fs->block_map.bitmap, sizeof(char), BLOCK_COUNT / 8, fs->fs_file);
}


int fs_load_inode_by_path(mjfs_fs_t * fs, const char * path, inode_t * inode) {
  int32_t inode_id = fs_find_inode_by_path(fs, path);
  if (ENOTFOUND == inode_id) {
    return ENOTFOUND;
  }

  fs_load_inode(fs, inode_id, inode);
  return 0;
}

int fs_load_inode(mjfs_fs_t * fs, int32_t inode_id, inode_t * inode) {
  int32_t load_from = inode_id * INODE_SIZE + INODE_SECTION;

  if (load_from >= BLOCK_SECTION) {
    return ENOINODE;
  }

  fseek(fs->fs_file, load_from, SEEK_SET);
  fread(inode, sizeof(inode_t), 1, fs->fs_file);
  return 0;
}

int fs_dump_inode(mjfs_fs_t * fs, int32_t inode_id, inode_t * inode) {
  int32_t load_from = inode_id * INODE_SIZE + INODE_SECTION;

  if (load_from >= BLOCK_SECTION) {
    return ENOINODE;
  }

  fseek(fs->fs_file, load_from, SEEK_SET);
  fwrite(inode, sizeof(inode_t), 1, fs->fs_file);
  return 0;
}

int32_t fs_find_inode_by_path(mjfs_fs_t * fs, const char * path) {
  path_t split_path = path_init(path);
  int32_t inode_id = ENOTFOUND;
  if (path[0] != '/') {
    inode_id = fs_find_recursive(fs, &fs->current_directory, &split_path, 0);
  } else {
    inode_t root;
    if (ENOINODE == fs_load_inode(fs, ROOT_INODE, &root)) {
      return ENOTFOUND;
    }
    inode_id = fs_find_recursive(fs, &root, &split_path, 0);
  }

  path_destroy(&split_path);
  return inode_id;
}

int32_t fs_find_recursive(mjfs_fs_t * fs, inode_t * inode, path_t * path, int path_part) {
  int32_t inode_id = fs_find_inode_in_dir(fs, inode, path->tokens[path_part]);
  if (ENOTFOUND == inode_id || ENOTADIR == inode_id) {
    return ENOTFOUND;
  }
  if (path->tokens[path_part + 1] == NULL) {
    return inode_id;
  }

  inode_t current_inode;
  if (ENOINODE == fs_load_inode(fs, inode_id, &current_inode)) {
    return ENOTFOUND;
  }
  return fs_find_recursive(fs, &current_inode, path, path_part + 1);
}

int32_t fs_find_inode_in_dir(mjfs_fs_t * fs, inode_t * dir, const char * name) {
  if (!dir->is_dir) {
    return ENOTADIR;
  }

  direntry_iter_t direntry_iter = direntry_iter_init(fs, dir);
  while (EEOFILE != direntry_next(fs, &direntry_iter)) {
    if (strcmp(direntry_iter.direntry.name, name) == 0) {
      int inode_id = direntry_iter.direntry.inode_id;
      direntry_iter_destroy(&direntry_iter);
      return inode_id;
    }
  }

  return ENOTFOUND;
}

int fs_delete_direntry_by_name(mjfs_fs_t * fs, inode_t * dir, const char * name) {
  direntry_t dummy;
  memset(&dummy, 0, sizeof(direntry_t));

  direntry_iter_t direntry_iter = direntry_iter_init(fs, dir);
  while (EEOFILE != direntry_next(fs, &direntry_iter)) {
    if (strcmp(direntry_iter.direntry.name, name) == 0) {

      fs_write(fs, dir, &dummy, sizeof(direntry_t), direntry_iter.direntry_offset - sizeof(direntry_t));

      direntry_iter_destroy(&direntry_iter);
      return 0;
    }
  }

  return ENOTFOUND;
}

int fs_delete_file_inode(mjfs_fs_t * fs, inode_t * deleted) {
  for (int i = 0; i < deleted->block_count; ++i) {
    fs_free_block(fs, deleted->address[i]);
  }
  fs_dump_inode(fs, deleted->id, deleted);

  fs_free_inode(fs, deleted->id);
  return 0;
}

int fs_delete_from_dir_by_name(mjfs_fs_t * fs, inode_t * dir, const char * name, bool recursive) {
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    return 0;
  }

  int32_t inode_id = fs_find_inode_in_dir(fs, dir, name);
  if (ENOTFOUND == inode_id) {
    return ENOTFOUND;
  } else if (ENOTADIR == inode_id) {
    return ENOTADIR;
  }
  inode_t deleted;
  fs_load_inode(fs, inode_id, &deleted);


  if (!deleted.is_dir) {
    fs_delete_direntry_by_name(fs, dir, name);
    fs_delete_file_inode(fs, &deleted);
    return 0;
  } else if (deleted.is_dir && recursive) {
    direntry_iter_t direntry_iter = direntry_iter_init(fs, &deleted);
    while (EEOFILE != direntry_next(fs, &direntry_iter)) {
      fs_delete_from_dir_by_name(fs, &deleted, direntry_iter.direntry.name, true);
    }
    fs_delete_file_inode(fs, &deleted);
  } else if (deleted.is_dir && !recursive) {
    return ENOTADIR;
  }
}


//upd дальше страшно
int fs_write(mjfs_fs_t * fs, inode_t * inode, void * buf, uint32_t buf_len, uint32_t offset) {
  int block_id;
  int block_offset;
  int written_bytes = 0;
  int current_offset = offset;

  while (written_bytes < buf_len) {
    block_id = inode_block_id_by_offset(inode, current_offset);
    if (block_id == EREQBLOCK) {
      int free_block = bitmap_find_free(fs->block_map.bitmap);
      if (ENOSPACE == free_block) {
        return ENOSPACE;
      }

      fs_lock_block(fs, free_block);
      if (ENOSPACE == inode_add_block(inode, free_block)) {
        return ENOSPACE;
      }
      block_id = free_block;
    }
    block_offset = inode_block_offset_by_offset(inode, current_offset);

    int write_to = BLOCK_SECTION + block_id * BLOCK_SIZE + block_offset;

    fseek(fs->fs_file, write_to, SEEK_SET);
    int need_write = (buf_len - written_bytes < BLOCK_SIZE - block_offset) ? (buf_len - written_bytes) : (BLOCK_SIZE -
                                                                                                          block_offset);
    fwrite(buf + written_bytes, sizeof(char), need_write, fs->fs_file);
    written_bytes += need_write;
    current_offset += written_bytes;
    inode->file_size += (offset + written_bytes > inode->file_size) ? written_bytes : 0;
  }

  inode_update_timestamp(inode);
  fs_dump_inode(fs, inode->id, inode);
  fs_load_inode(fs, inode->id, inode);

  return written_bytes;
}

int fs_write_to_dir(mjfs_fs_t * fs, inode_t * dir, direntry_t direntry) {
  uint32_t offset = 0;
  direntry_t empty_direntry;
  while (EEOFILE != fs_read(fs, dir, &empty_direntry, sizeof(direntry_t), offset)) {
    if (empty_direntry.name_len == 0) {
      break;
    }
    offset += sizeof(direntry_t);
  }

  if (ENOSPACE == fs_write(fs, dir, &direntry, sizeof(direntry_t), offset)) {
    return ENOSPACE;
  }
  return 0;
}

int fs_read(mjfs_fs_t * fs, inode_t * inode, void * dest, uint32_t dest_len, uint32_t offset) {
  int block_id;
  int block_offset;

  int read_bytes = 0;
  int current_offset = offset;

  while (read_bytes < dest_len) {
    if (current_offset >= inode->file_size) {
      return EEOFILE;
    }

    block_id = inode_block_id_by_offset(inode, current_offset);
    if (block_id == EREQBLOCK) {
      return EEOFILE;
    }
    block_offset = inode_block_offset_by_offset(inode, current_offset);
    if (block_id == EREQBLOCK) {
      return EEOFILE;
    }

    int read_from = BLOCK_SECTION + block_id * BLOCK_SIZE + block_offset;
    fseek(fs->fs_file, read_from, SEEK_SET);
    int need_write = (dest_len - read_bytes < BLOCK_SIZE - block_offset) ? (dest_len - read_bytes) : (BLOCK_SIZE -
                                                                                                      block_offset);
    fread(dest + read_bytes, sizeof(char), need_write, fs->fs_file);

    read_bytes += need_write;
    current_offset += read_bytes;
  }

  return read_bytes;
}

int fs_read_from_dir(mjfs_fs_t * fs, inode_t * dir, uint32_t offset, direntry_t * direntry) {
  uint32_t new_offset = offset;
  do {
    if (EEOFILE == fs_read(fs, dir, direntry, sizeof(direntry_t), new_offset)) {
      return EEOFILE;
    }
    new_offset += sizeof(direntry_t);
  } while (direntry->name_len == 0);
  return new_offset - offset;
}

int fs_reload_current_dir(mjfs_fs_t * fs) {
  fs_load_inode(fs, fs->current_directory.id, &fs->current_directory);
}

int fs_lookup_directory(mjfs_fs_t * fs, const char * directory) {
  inode_t dir;
  if (ENOTFOUND == fs_load_inode_by_path(fs, directory, &dir)) {
    return ENOTFOUND;
  }
  if (!dir.is_dir) {
    return ENOTADIR;
  }

  direntry_iter_t direntry_iter = direntry_iter_init(fs, &dir);
  while (EEOFILE != direntry_next(fs, &direntry_iter)) {
    //char * inode_info = inode_info_str(&direntry_iter.direntry.);
    printf("%s\n", direntry_iter.direntry.name);
    //free(inode_info);
  }

  return 0;
}

int fs_set_current_directory(mjfs_fs_t * fs, const char * directory) {
  if (ENOTFOUND == fs_load_inode_by_path(fs, directory, &fs->current_directory)) {
    return ENOTFOUND;
  } else if (!fs->current_directory.is_dir) {
    fs_load_inode_by_path(fs, fs->current_directory_name, &fs->current_directory);
    return ENOTADIR;
  }

  if (directory[0] == '/') {
    free(fs->current_directory_name);
    fs->current_directory_name = calloc(sizeof(directory), 1);
    strcpy(fs->current_directory_name, directory);
  } else if (strcmp(directory, ".") == 0) {

  } else if (strcmp(directory, "..") == 0) {
    char * parent_path = path_get_parent(fs->current_directory_name);
    free(fs->current_directory_name);
    if (strlen(parent_path) == 0) strcat(parent_path, "/\0");
    fs->current_directory_name = parent_path;
  } else {
    char * new_directory_path = calloc(sizeof(directory) + sizeof(fs->current_directory_name) + 1, 1);
    strcat(new_directory_path, fs->current_directory_name);
    if (fs->current_directory_name[strlen(fs->current_directory_name) - 1] != '/') strcat(new_directory_path, "/");
    strcat(new_directory_path, directory);

    free(fs->current_directory_name);
    fs->current_directory_name = new_directory_path;
  }
  return 0;
}

char * fs_pwd(mjfs_fs_t * fs) {
  return fs->current_directory_name;
}

int fs_create_directory(mjfs_fs_t * fs, const char * directory) {
  char * dir_name = path_get_last(directory);

  if (strlen(dir_name) > sizeof(direntry_t) - 2 * sizeof(int32_t)) {
    free(dir_name);
    return ENOSPACE;
  }

  inode_t parent;
  if (directory[0] != '/') {
    parent = fs->current_directory;
  } else {
    char * parent_path = path_get_parent(directory);
    if (ENOTFOUND == fs_load_inode_by_path(fs, parent_path, &parent)) {
      free(parent_path);
      return ENOTFOUND;
    }
    free(parent_path);

    if (!parent.is_dir) {
      return ENOTADIR;
    }
  }

  if (ENOTFOUND != fs_find_inode_by_path(fs, directory)) {
    return EALRDEX;
  }

  inode_t dir;
  if (ENOSPACE == fs_init_new_inode(fs, &dir)) {
    return ENOSPACE;
  }
  dir.is_dir = true;

  if (ENOSPACE == fs_write_to_dir(fs, &dir, direntry_init(dir.id, "."))) {
    return ENOSPACE;
  }
  if (ENOSPACE == fs_write_to_dir(fs, &dir, direntry_init(parent.id, ".."))) {
    return ENOSPACE;
  }
  if (ENOSPACE == fs_write_to_dir(fs, &parent, direntry_init(dir.id, dir_name))) {
    return ENOSPACE;
  }
  fs_dump_inode(fs, dir.id, &dir);
  fs_dump_inode(fs, parent.id, &parent);
  fs_reload_current_dir(fs);

  free(dir_name);
  return 0;
}

int fs_create_file(mjfs_fs_t * fs, const char * file) {
  char * file_name = path_get_last(file);

  if (strlen(file_name) > sizeof(direntry_t) - 2 * sizeof(int32_t)) {
    free(file_name);
    return ENOSPACE;
  }

  inode_t parent;
  if (file[0] != '/') {
    parent = fs->current_directory;
  } else {
    char * parent_path = path_get_parent(file);
    if (ENOTFOUND == fs_load_inode_by_path(fs, parent_path, &parent)) {
      free(parent_path);
      return ENOTFOUND;
    }
    free(parent_path);

    if (!parent.is_dir) {
      return ENOTADIR;
    }
  }

  if (ENOTFOUND != fs_find_inode_by_path(fs, file)) {
    return EALRDEX;
  }

  inode_t file_inode;
  if (ENOSPACE == fs_init_new_inode(fs, &file_inode)) {
    return ENOSPACE;
  }
  file_inode.is_dir = false;

  if (ENOSPACE == fs_write_to_dir(fs, &parent, direntry_init(file_inode.id, file_name))) {
    return ENOSPACE;
  }
  fs_dump_inode(fs, file_inode.id, &file_inode);
  fs_reload_current_dir(fs);

  free(file_name);
  return 0;
}

int fs_delete_directory(mjfs_fs_t * fs, const char * directory) {
  inode_t parent;
  if (directory[0] != '/') {
    parent = fs->current_directory;
  } else {
    char * parent_path = path_get_parent(directory);
    if (ENOTFOUND == fs_load_inode_by_path(fs, parent_path, &parent)) {
      free(parent_path);
      return ENOTFOUND;
    }
    free(parent_path);

    if (!parent.is_dir) {
      return ENOTADIR;
    }
  }

  char * dir_name = path_get_last(directory);
  int result = fs_delete_from_dir_by_name(fs, &parent, dir_name, true);
  fs_reload_current_dir(fs);
  free(dir_name);
  return result;
}

int fs_delete_file(mjfs_fs_t * fs, const char * file) {
  inode_t parent;
  if (file[0] != '/') {
    parent = fs->current_directory;
  } else {
    char * parent_path = path_get_parent(file);
    if (ENOTFOUND == fs_load_inode_by_path(fs, parent_path, &parent)) {
      free(parent_path);
      return ENOTFOUND;
    }
    free(parent_path);

    if (!parent.is_dir) {
      return ENOTADIR;
    }
  }

  char * file_name = path_get_last(file);
  int result = fs_delete_from_dir_by_name(fs, &parent, file_name, false);
  fs_reload_current_dir(fs);
  free(file_name);
  return result;
}

int fs_load_to(mjfs_fs_t * fs, const char * external, const char * file) {
   inode_t file_inode;
   if (ENOTFOUND == fs_load_inode_by_path(fs, file, &file_inode)) {
     return ENOTFOUND;
   } else if (file_inode.is_dir) {
     return ENOTADIR;
   }

   int fd = open(external, O_RDONLY);
   if (-1 == fd) {
     return EFILE;
   }

   struct stat external_stat;
   if (-1 == fstat(fd, &external_stat)) {
     return EFILE;
   }

   int bytes = 0;
   char buffer[16]; // TODO read/write constant
   while (bytes < external_stat.st_size) {
     int read_bytes = read(fd, buffer, 16);

     if (ENOSPACE == fs_write(fs, &file_inode, &buffer, read_bytes, bytes)) {
       close(fd);
       return ENOSPACE;
     }
     bytes += read_bytes;
   }

   close(fd);
   return 0;
}

int fs_store_from(mjfs_fs_t * fs, const char * file, const char * external) {
  inode_t file_inode;
  if (ENOTFOUND == fs_load_inode_by_path(fs, file, &file_inode)) {
    return ENOTFOUND;
  } else if (file_inode.is_dir) {
    return ENOTADIR;
  }

  int fd = open(external, O_CREAT | O_WRONLY, 0666);
  if (-1 == fd) {
    return EFILE;
  }
  if (-1 == truncate(external, file_inode.file_size)) {
    return EFILE;
  }

  int bytes = 0;
  char buffer[16];
  while (bytes < file_inode.file_size) {
    int read_bytes = fs_read(fs, &file_inode, buffer, 16, bytes);

    bytes += write(fd, buffer, read_bytes);
  }

  close(fd);
  return 0;
}

int fs_cat(mjfs_fs_t * fs, const char * file) {
  inode_t file_inode;
  if (ENOTFOUND == fs_load_inode_by_path(fs, file, &file_inode)) {
    return ENOTFOUND;
  } else if (file_inode.is_dir) {
    return ENOTADIR;
  }

  int bytes;
  char buffer[16];
  while (bytes < file_inode.file_size) {
    int read_bytes = fs_read(fs, &file_inode, buffer, 16, bytes);

    printf("%.*s", read_bytes, buffer);
    bytes += read_bytes;
  }

  return 0;
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


/*----------d i r  e n r t y----------*/
direntry_iter_t direntry_iter_init(mjfs_fs_t * fs, inode_t * dir_inode) {
  direntry_iter_t direntry_iter;

  direntry_iter.dir_inode = *dir_inode;

  direntry_iter.next_direntry = calloc(sizeof(direntry_t), 1);

  if (direntry_iter.dir_inode.is_dir) {
    int32_t shift = fs_read_from_dir(fs, &direntry_iter.dir_inode, 0, direntry_iter.next_direntry);
    if (EEOFILE == shift) {
      direntry_iter.next_direntry = NULL;
    } else {
      direntry_iter.direntry_offset = 0;
      direntry_iter.read_offset = shift;
    }
  } else {
    direntry_iter.next_direntry = NULL;
  }

  return direntry_iter;
}

int direntry_next(mjfs_fs_t * fs, direntry_iter_t * direntry_iter) {
  if (direntry_iter->next_direntry == NULL) {
    return EEOFILE;
  }

  direntry_iter->direntry = *direntry_iter->next_direntry;

  free(direntry_iter->next_direntry);
  direntry_iter->next_direntry = calloc(sizeof(direntry_iter_t), 1);

  int shift = fs_read_from_dir(fs, &direntry_iter->dir_inode, direntry_iter->read_offset, direntry_iter->next_direntry);
  if (EEOFILE == shift) {
    direntry_iter->next_direntry = NULL;
  } else {
    direntry_iter->direntry_offset = direntry_iter->read_offset;
    direntry_iter->read_offset += shift;
  }

  return 0;
}

void direntry_iter_destroy(direntry_iter_t * direntry_iter) {
  if (direntry_iter->next_direntry != NULL) {
    free(direntry_iter->next_direntry);
  }
}
