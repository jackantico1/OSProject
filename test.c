#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // Create pipe
  int fds[2];
  int err = pipe(fds);
  if (err == -1) {
    perror("pipe");
    return 1; 
  }
  // Spawn child
  int ret = fork();
  if (ret < 0) {
    perror("fork");
    return 1; 
  } else if (ret == 0) {
    // Close write end of pipe
    close(fds[1]);
    // Duplicate read end of pipe in standard input
    close(0);
    dup(fds[0]);
    // Child launches command "wc"
    char *argv[2];
    argv[0] = "wc";
    argv[1] = NULL;
    execvp(argv[0], argv);
  } else {
    // Close read end of pipe
    close(fds[0]);
    // Duplicate write end of pipe in standard output
    close(1);
    dup(fds[1]);
    // Parent launches command "ls -l"
    char *argv[3];
    argv[0] = "ls";
    argv[1] = "-l";
    argv[2] = NULL;
    execvp(argv[0], argv);
  }
  return 0; 
}