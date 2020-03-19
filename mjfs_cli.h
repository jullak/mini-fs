#ifndef MINI_FS_MJFS_CLI_H
#define MINI_FS_MJFS_CLI_H

int handler(char * cmd, char * argv[]);

int run_cli();

void print_fs_list();
char * build_fsfile_path(char * file);
char * get_fs_location_path();
void init_cmd_argv_from_line(char * line, char ** cmd, char ** argv[]);


#endif //MINI_FS_MJFS_CLI_H
