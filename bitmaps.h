#ifndef MINI_FS_BITMAPS_H
#define MINI_FS_BITMAPS_H

#include <stdbool.h>
#include <stdint.h>

#include "fs_constants.h"


typedef struct inode_map {
  char bitmap[INODE_COUNT / 8];
} inode_map_t;

typedef struct block_map {
  char bitmap[BLOCK_COUNT / 8];
} block_map_t;

int32_t bitmap_find_free(char * bitmap); // ENOSPACE
int32_t bitmap_set_lock(char * bitmap, int32_t id); // ELOCK
int32_t bitmap_set_free(char * bitmap, int32_t id);

bool bitmap_is_lock(char * bitmap, int32_t id);


#endif //MINI_FS_BITMAPS_H