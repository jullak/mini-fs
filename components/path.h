#ifndef MINI_FS_PATH_H
#define MINI_FS_PATH_H


typedef struct path {
  char * full_path;
  char ** tokens;
  int tokens_num;
} path_t;

path_t path_init(const char * path); // need to be free by destroy
void path_destroy(path_t * path);

char * path_get_parent(const char * path); // need to be free
char * path_get_last(const char * path); // need to be free


#endif //MINI_FS_PATH_H