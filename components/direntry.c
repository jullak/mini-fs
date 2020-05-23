#include <string.h>

#include "direntry.h"


direntry_t direntry_init(int32_t inode_id, const char * name) {
  direntry_t direntry;
  direntry.inode_id = inode_id;
  direntry.name_len = strlen(name);
  strcpy(direntry.name, name);

  return direntry;
}