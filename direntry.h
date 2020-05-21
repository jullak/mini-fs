#ifndef MINI_FS_DIRENTRY_H
#define MINI_FS_DIRENTRY_H

#include <stdint.h>


#define DIRENTRY_SIZE 24

typedef struct direntry {
  int32_t inode_id;
  uint32_t name_len;
  char name[DIRENTRY_SIZE - 8]; // no absolute
} direntry_t;

direntry_t direntry_init(int32_t inode_id, const char * name); // does not to be free


#endif //MINI_FS_DIRENTRY_H