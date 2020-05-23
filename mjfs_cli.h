#ifndef MINI_FS_MJFS_CLI_H
#define MINI_FS_MJFS_CLI_H


int command(char * cmd, char * argv[], FILE * file);

int run_cli();
int run_server_cli();

void print_fs_list(FILE * file); // does not to be free
void print_help(FILE * file); // does not to be free
char * build_fsfile_path(char * file); // need to be free
char * get_fs_location_path(); // need to be free
void init_cmd_argv_from_line(char * line, char ** cmd, char ** argv[]); // does not to be free
int is_fs_init();

int setup_server();

#endif //MINI_FS_MJFS_CLI_H