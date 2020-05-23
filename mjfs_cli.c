#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "cli_commands.h"
#include "errors.h"
#include "mjfs_cli.h"
#include "mini_fs.h"


#define LOCATION_MJFS "/.mjfs"

static mjfs_fs_t fs;
bool exited = false;

int command(char * cmd, char * args[], FILE * file) {
  if (strcmp(cmd, MAKE_MJFS) == 0) {
    //0 - fs file name
    char * location = build_fsfile_path(args[0]);

    if (EFILE == fs_init(location)) {
      fprintf(file, "Couldn't create fs file\n");
    } else {
      print_fs_list(file);
    }

    free(location);
  } else if (strcmp(cmd, SELECT_MJFS) == 0) {
    //0 - fs file name
    char * location = build_fsfile_path(args[0]);

    int result = fs_mount(&fs, location);
    if (EFILE == result) {
      fprintf(file, "Wrong fs file\n");
      fs = init_dummy_fs();
    } else if (ENOSPACE == result) {
      fprintf(file, "Very little fs file\n");
      fs = init_dummy_fs();
    } else {
      fs_lookup_directory(&fs, ".", file);
    }

    free(location);
  } else if (strcmp(cmd, LIST_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    //0 - fs dir name
    int result = 0;
    if (args[0] == NULL) {
      result = fs_lookup_directory(&fs, ".", file);
    } else {
      result = fs_lookup_directory(&fs, args[0], file);
    }
    if (ENOTFOUND == result) {
      fprintf(file, "Directory does not exist\n");
    }
  } else if (strcmp(cmd, PWD_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    //no args
    fprintf(file, "%s\n", fs_pwd(&fs));
  } else if (strcmp(cmd, CHANGE_DIR_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    //0 - new directory name
    int result = fs_set_current_directory(&fs, args[0]);
    if (ENOTFOUND == result) {
      fprintf(file, "Directory does not exist\n");
    } else if (ENOTADIR == result) {
      fs_set_current_directory(&fs, args[0]);
    }
  } else if (strcmp(cmd, MOVE_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    // 0 - source
    // 1 - dest
    int result = fs_mv(&fs, args[0], args[1]);
    if (ENOSPACE == result ) {
      fprintf(file, "No free space in FS\n");
    } else if (ENOTFOUND == result) {
      fprintf(file, "Some dir in path didn't find\n");
    } else if (ENOTADIR == result) {
      fprintf(file, "Some in path isn't dir\n");
    } else if (EALRDEX == result) {
      fprintf(file, "File or dir with destination name is already exist");
    }
  } else if (strcmp(cmd, MAKE_DIR_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    // 0 - new directory name or full path to new dir (but don't create recursively)
    int result = fs_create_directory(&fs, args[0]);
    if (ENOSPACE == result) {
      fprintf(file, "No free space in FS\n");
    } else if (ENOTFOUND == result) {
      fprintf(file, "Some dir in path didn't find\n");
    } else if (ENOTADIR == result) {
      fprintf(file, "Some in path isn't dir\n");
    } else if (EALRDEX == result) {
      fprintf(file, "File or dir with that name is already exist");
    }
  } else if (strcmp(cmd, CREATE_FILE_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    // 0 - new file name or full path to new file (but don't create recursively)
    int result = fs_create_file(&fs, args[0]);
    if (ENOSPACE == result) {
      fprintf(file, "No free space in FS\n");
    } else if (ENOTFOUND == result) {
      fprintf(file, "Some dir in path didn't find\n");
    } else if (ENOTADIR == result) {
      fprintf(file, "Some in path isn't dir\n");
    } else if (EALRDEX == result) {
      fprintf(file, "File or dir with that name is already exist");
    }
  } else if (strcmp(cmd, REMOVE_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    // 0 - file name
    // 1 - recursive flag
    int result;
    if (args[1] == NULL) {
      result = fs_delete_file(&fs, args[0]);
    } else {
      result = fs_delete_directory(&fs, args[0]);
    }

    if (ENOTFOUND == result) {
      fprintf(file, "Some dir in path didn't find\n");
    } else if (ENOTADIR == result) {
      fprintf(file, "Some in path isn't dir\n");
    }
  } else if (strcmp(cmd, CAT_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    // 0 - file name
    int result = fs_cat(&fs, args[0], file);
    if (ENOTFOUND == result) {
      fprintf(file, "Some dir in path didn't find\n");
    } else if (ENOTADIR == result) {
      fprintf(file, "Cat directory!\n");
    }
  } else if (strcmp(cmd, EXTERNAL_LOAD_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    // 0 - file on host
    // 1 - file on mjfs
    int result = fs_load_to(&fs, args[0], args[1]);
    if (EFILE == result) {
      fprintf(file, "Problem with file on host\n");
    } else if (ENOSPACE == result) {
      fprintf(file, "No free space in FS\n");
    } else if (ENOTADIR == result) {
      fprintf(file, "Write to dir!\n");
    } else if (ENOTFOUND == result) {
      fprintf(file, "File didn't find in FS\n");
    }
  } else if (strcmp(cmd, EXTERNAL_STORE_MJFS) == 0) {
    if (!is_fs_init()) {
      fprintf(file, "There is no selected fs! Please, select one or create new\n");
      return 0;
    }

    // 0 - file on mjfs
    // 1 - file on host
    int result = fs_store_from(&fs, args[0], args[1]);
    if (EFILE == result) {
      fprintf(file, "Problem with file on host\n");
    } else if (ENOTADIR == result) {
      fprintf(file, "Write to dir!\n");
    } else if (ENOTFOUND == result) {
      fprintf(file, "File didn't find in FS\n");
    }
  } else if (strcmp(cmd, UNMOUNT_MJFS) == 0) {
    if (fs.fs_file) {
      fs_unmount(&fs);
      fs = init_dummy_fs();
    }
    print_fs_list(file);
  } else if (strcmp(cmd, HELP_MJFS) == 0) {
    print_help(file);
  } else if (strcmp(cmd, EXIT) == 0) {
    if (fs.fs_file) {
      fs_unmount(&fs);
      fs = init_dummy_fs();
    }
    exited = true;
    fprintf(file, "Goodbye\n");
  } else {
    fprintf(file, "Wrong command\n");
  }

  return 0;
}

int run_cli() {
  fs = init_dummy_fs();

  printf("Welcome to miniJFS!\n");
  printf("Choose your fs-file or create new:\n\n");
  print_fs_list(stdout);
  print_help(stdout);

  printf("$ ");

  char * line = NULL;
  size_t line_len = 0;
  char * cmd;
  char ** argv = calloc(10, sizeof(void *)); //TODO: constant

  while (!exited && getline(&line, &line_len, stdin)) {
    init_cmd_argv_from_line(line, &cmd, &argv);

    command(cmd, argv, stdout);

    free(line);

    line = NULL;
    line_len = 0;
    printf("$ ");
    fflush(stdout);
  }

  return 0;
}
int run_server_cli() {
  fs = init_dummy_fs();

  int fd = setup_server();
  char buffer_in[256];
  memset(buffer_in, 0, 256);

  char * cmd;
  char ** argv = calloc(10, sizeof(void *));

  while (1) {
    struct sockaddr_in client_addr;
    int addr_size = sizeof(client_addr);

    int client_fd = accept(fd, (struct sockaddr*) &client_addr, &addr_size);
    if (0 > client_fd) continue;

    FILE * client = fdopen(client_fd, "r+");

    while (!exited && (read(client_fd, buffer_in, 4096))) {
      init_cmd_argv_from_line(buffer_in, &cmd, &argv);

      command(cmd, argv, client);
      fprintf(client, " ");
      fflush(client);

      memset(buffer_in, 0, 256);
    }

    shutdown(client_fd, SHUT_RDWR);
    fclose(client);
    close(client_fd);
  }

  return 0;
}

int setup_server() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (0 > fd) {
    return -1; // fail
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof (addr));

  addr.sin_family = AF_INET;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(7795);

  if (0 > bind(fd, (struct sockaddr * ) &addr, sizeof(addr))) {
    return -1;
  }
  if (0 > listen(fd, 10)) {
    return -1;
  }

//  int val = 1;
//  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  return fd;
}

void print_fs_list(FILE * file) {
  char * location = get_fs_location_path();
  DIR * fsdir = opendir(location);

  if (!fsdir && ENOENT == errno) {
    if (0 != mkdir(location, 0777)) {
      printf("Something going wrong, please, try again\n");
    }

    fsdir = opendir(LOCATION_MJFS);
  }

  fprintf(file, "Available fs:\n\n");

  struct dirent * current_entry;
  while ((current_entry = readdir(fsdir)) != NULL) {
    if (strcmp(current_entry->d_name, ".") == 0
        || strcmp(current_entry->d_name, "..") == 0) {
      continue;
    }
    fprintf(file, "\t%s\n", current_entry->d_name);
  }
  fprintf(file, "\n");

  closedir(fsdir);
  free(location);
}
void print_help(FILE * file) {
  fprintf(file, "\n\n"
         "makemjfs fsname\t create new fs file\n"
         "select fsname\t select current fs file\n"
         "ls [dir name]\t list of file in directory\n"
         "pwd\t show current path\n"
         "cd dirname\t change current directory\n"
         "rm filename [r]\t remove file or directory (only with r flag!)\n"
         "load path_host path_mjfs\t load file from host to mjfs\n"
         "store path_mjfs path_host\t store file from mjfs to host\n");

}
char * build_fsfile_path(char * file) {
  char * fs_path = get_fs_location_path();
  char * location = calloc(strlen(fs_path) + strlen(file) + 2, 1);
  strcat(location, fs_path);
  strcat(location, "/");
  strcat(location, file);
  strcat(location, "\0");

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
int is_fs_init() {
  return fs.fs_file != NULL;
}