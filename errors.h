#ifndef MINI_FS_ERRORS_H
#define MINI_FS_ERRORS_H


enum error {
  EFILE = -1, // problem with file on host
  ENOSPACE = -2, // no space in fs
  ELOCK = -3, // problem with block/inode lock
  EREQBLOCK = -4, // block is require
  ENOINODE = -5, // no such inode
  ENOTFOUND = -6, // file/dir not found
  ENOTADIR = -7, // operation with dir but passed file
  EEOFILE = -9, // it is end of file (on read)
  EALRDEX = -10,
};


#endif //MINI_FS_ERRORS_H