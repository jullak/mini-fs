#ifndef MINI_FS_FS_CONSTANTS_H
#define MINI_FS_FS_CONSTANTS_H


//byte
#define FS_SIZE 8192

//tmp
#define BLOCK_SIZE 128
#define INODE_SIZE 64
#define BLOCK_COUNT 16
#define INODE_COUNT 8

#define BLOCK_IN_FILE 4

#define BLOCK_SECTION sizeof(superblock_t) + INODE_COUNT / 8 + BLOCK_COUNT / 8 + INODE_COUNT * INODE_SIZE
#define INODE_SECTION sizeof(superblock_t) + INODE_COUNT / 8 + BLOCK_COUNT / 8;


#endif //MINI_FS_FS_CONSTANTS_H