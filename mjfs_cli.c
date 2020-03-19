#include "mjfs_cli.h"
#include "mini_fs.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

/*---------- C O M M A N D S ----------*/
#define MAKE_MJFS "makemjfs"
#define SELECT_MJFS "select"
#define LIST_MJFS "ls"
#define MAKE_DIR_MJFS "mkdir"
#define CHANGE_DIR_MJFS "cd"
#define CREATE_FILE_MJFS "touch"
#define CAT_MJFS "cat"
#define UNMOUNT_MJFS "unmount"
#define EXIT "exit"
/*-------------------------------------*/

#define LOCATION_MJFS "/.mjfs"


static mjfs_fs_t fs;
bool exited = false;

int handler(char * cmd, char * args[]) {
    if (strcmp(cmd, MAKE_MJFS) == 0) {
        //0 - fs file name
        char * location = build_fsfile_path(args[0]);

        if (EFILE == fs_init(location)) {
            printf("Couldn't create fs file\n");
        } else {
            print_fs_list();
        }

        free(location);
    } else if (strcmp(cmd, SELECT_MJFS) == 0) {
        //0 - fs file name
        char * location = build_fsfile_path(args[0]);

        if (EFILE == fs_mount(&fs, location)) {
            printf("Wrong fs file\n");
        }
        //ls

        free(location);
    } else if (strcmp(cmd, LIST_MJFS) == 0) {
        //fs_lookup_directory
    } else if (strcmp(cmd, MAKE_DIR_MJFS) == 0) {
        //fs_create_directory
    } else if (strcmp(cmd, CHANGE_DIR_MJFS) == 0) {
        //fs_change_dicretocry
    } else if (strcmp(cmd, CREATE_FILE_MJFS) == 0) {
        //fs_create_file
    } else if (strcmp(cmd, CAT_MJFS) == 0) {
        //fs_read_alll
    } else if (strcmp(cmd, UNMOUNT_MJFS) == 0) {
      fs_unmount(&fs);
      print_fs_list();
    } else if (strcmp(cmd, EXIT) == 0) {
        if (fs.fs_file) {
            fs_unmount(&fs);
        }
        exited = true;
        printf("Goodbye\n");
    } else {
        //unknown command
    }

    //fs_cp_host_fs
    //fs_cp_fs_host

    //fs_open_file
    //fs_write
    //

    //кто ответственнен за запись в блок? inode or block or fs???

    return 0;
}

int run_cli() {
    fs = init_dummy_fs();

    printf("Welcome to miniJFS!\n");
    printf("Choose your fs-file or create new:\n\n");
    print_fs_list();
    printf("\n\n"
           "makemjfs fsname\t create new fs file\n"
           "select fsname\t select current fs file\n"
           "$ ");

    char * line = NULL;
    size_t line_len = 0;
    char * cmd;
    char ** argv = calloc(10, sizeof(void *)); //TODO: constant

    while (!exited && getline(&line, &line_len, stdin)) {
        init_cmd_argv_from_line(line, &cmd, &argv);

        handler(cmd, argv);

        free(line);

        line = NULL;
        line_len = 0;
        printf("$ ");
        fflush(stdout);
    }

    return 0;
}


void print_fs_list() {
    char * location = get_fs_location_path();
    DIR * fsdir = opendir(location);

    if (!fsdir && ENOENT == errno) {
        if (0 != mkdir(location, 0777)) {
            printf("Something going wrong, please, try again\n");
        }

        fsdir = opendir(LOCATION_MJFS);
    }

    printf("Available fs:\n\n");

    struct dirent * current_entry;
    while (current_entry = readdir(fsdir)) {
        if (strcmp(current_entry->d_name, ".") == 0
            || strcmp(current_entry->d_name, "..") == 0) {
            continue;
        }
        printf("\t%s\n", current_entry->d_name);
    }
    printf("\n");

    closedir(fsdir);
    free(location);
}

char * build_fsfile_path(char * file) {
    char * fs_path = get_fs_location_path();
    char * location = calloc(strlen(fs_path) + strlen(file) + 1, 1);
    strcat(location, fs_path);
    strcat(location, "/");
    strcat(location, file);

    free(fs_path);
    return location;
}

char * get_fs_location_path() {
    const char * homedir = getenv("HOME");

    char * location = calloc(strlen(homedir) + sizeof(LOCATION_MJFS), 1);
    strcat(location, homedir);
    strcat(location, LOCATION_MJFS);

    return location;
}

void init_cmd_argv_from_line(char * line, char ** cmd, char ** argv[]) {
    int token_count = 0;
    char * current_token = strtok(line, " \n");
    *cmd = current_token;
    while (current_token != NULL) {
        current_token = strtok(NULL, " \n");
        (*argv)[token_count] = current_token;
        ++token_count;
    }
}

