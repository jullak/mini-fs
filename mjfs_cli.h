#ifndef MINI_FS_MJFS_CLI_H
#define MINI_FS_MJFS_CLI_H


int handler(char * cmd, char * argv[]);

int run_cli();

void print_fs_list(); // does not to be free
void print_help(); // does not to be free
char * build_fsfile_path(char * file); // need to be free
char * get_fs_location_path(); // need to be free
void init_cmd_argv_from_line(char * line, char ** cmd, char ** argv[]); // does not to be free
int is_fs_init();

#endif //MINI_FS_MJFS_CLI_H