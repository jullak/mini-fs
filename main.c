#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "mjfs_cli.h"

pid_t daemonize() {
  pid_t pid = fork();

  if (0 > pid) {
    printf("Error while daemon start\n");
    return pid;
  } else if (0 == pid) {
    if (0 > setsid()) { //zombie?
      printf("Problem with new group\n");
      return -1;
    }

    pid_t pid_d = fork();
    if (0 > pid_d) return pid_d;
    if (0 < pid_d) return pid_d; //not returned?

    //umask(0);
    chdir("/");
    return pid_d;
  } else {
    return pid;
  }
}

int main(int argc, char * argv[]) {
  if (argc < 2) {
    run_cli();
  } else if (strcmp(argv[1], "-d") == 0) {
    pid_t daemon = daemonize();
    if (0 < daemon) return 0;
    if (0 > daemon) return daemon;

    run_server_cli();
  } else {
    printf("Wrong args\n");
  }
  return 0;
}
