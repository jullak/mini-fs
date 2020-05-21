#include <time.h>

#include "errors.h"
#include "fs_constants.h"
#include "inode.h"


int32_t inode_block_id_by_offset(inode_t * inode, int32_t offset) {
  uint32_t block_id = offset / BLOCK_SIZE;
  if (offset > inode->block_count * BLOCK_SIZE || block_id >= inode->block_count) {
    return EREQBLOCK;
  }
  return inode->address[block_id];
}

int32_t inode_block_offset_by_offset(inode_t * inode, int32_t offset) {
  if (offset > inode->block_count * BLOCK_SIZE || offset / BLOCK_SIZE >= inode->block_count) {
    return EREQBLOCK;
  }
  return offset % BLOCK_SIZE;
}

int32_t inode_add_block(inode_t * inode, int32_t block_id) { //нет косвенной адресации
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