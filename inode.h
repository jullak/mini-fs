#ifndef MINI_FS_INODE_H
#define MINI_FS_INODE_H

#include <stdbool.h>
#include <stdint.h>


typedef struct inode {
  int32_t id;
  int32_t file_size;
  int32_t block_count;
  time_t timestamp;
  bool is_dir;
  int32_t address[BLOCK_IN_FILE];
} inode_t;

int32_t inode_block_id_by_offset(inode_t * inode, int32_t offset); // EREQBLOCK
int32_t inode_block_offset_by_offset(inode_t * inode, int32_t offset); // EREQBLOCK
int32_t inode_add_block(inode_t * inode, int32_t block_id); // ENOSPACE or block_id
void inode_update_timestamp(inode_t * inode);


char * inode_info_str(inode_t * inode); // need to be free


#endif //MINI_FS_INODE_H