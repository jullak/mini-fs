#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  printf("Welcome to miniJFS!\n");

  printf("\n\n"
         "makemjfs fsname\t create new fs file\n"
         "select fsname\t select current fs file\n"
         "ls [dir name]\t list of file in directory\n"
         "pwd\t show current path\n"
         "cd dirname\t change current directory\n"
         "rm filename [r]\t remove file or directory (only with r flag!)\n"
         "load path_host path_mjfs\t load file from host to mjfs\n"
         "store path_mjfs path_host\t store file from mjfs to host\n");

  printf("May the force be with you...\n");


  int sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  inet_aton("127.0.0.1", &(addr.sin_addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(7795);

  int connection = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  if (0 > connection) {
    close(sock);
    return -1;
  }


  printf("$ ");

  char * line = NULL;
  size_t line_len = 0;

  char answer[4096];

  while (getline(&line, &line_len, stdin)) {
    write(sock, line, line_len);

    int read_bytes = read(sock, answer, 4096);

    if (read_bytes < 4096) answer[read_bytes] = '\0';
    printf("%s", answer);

    printf("\n");
    printf("$ ");
    fflush(stdout);
  }

  shutdown(sock, SHUT_RDWR);
  close(sock);

  return 0;
}
