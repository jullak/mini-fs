#include <string.h>
#include <malloc.h>

#include "path.h"


path_t path_init(const char * path) {
  path_t new_path;
  int occurrence = 0;
  for (int ptr = 0; path[ptr] != '\0'; ++ptr, (path[ptr] == '/') ? ++occurrence : occurrence);

  new_path.full_path = calloc(strlen(path) + 1, 1);
  new_path.tokens = calloc(occurrence + 1, sizeof(char *));

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

char * path_get_parent(const char * path) {
  int end = strlen(path);
  int slash = end - 1;
  for (slash; slash >= 0 && path[slash] != '/'; --slash);

  char * parent_path = calloc(slash + 1, 1);
  for (int i = 0; i < slash; ++i) {
    parent_path[i] = path[i];
  }
  parent_path[slash] = '\0';
  return parent_path;
}

char * path_get_last(const char * path) {
  int end = strlen(path);
  int slash = end - 1;
  for (slash; slash >= 0 && path[slash] != '/'; --slash);

  if (slash == -1) slash = 0;
  char * last_path = calloc(end - slash, 1);
  strcpy(last_path, path + slash);
  return last_path;
}

