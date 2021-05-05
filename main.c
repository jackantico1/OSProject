#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#define MAX_SUB_COMMANDS 5
#define MAX_ARGS 10

struct SubCommand {
    char *line;
    char *argv[MAX_ARGS];
};
struct Command {
    struct SubCommand sub_commands[MAX_SUB_COMMANDS];
    int num_sub_commands;
    char *stdin_redirect;
    char *stdout_redirect;
    int background;
};

void ReadArgs(char *in, char **argv, int size) {
    int argc = 0;
    char *token = strtok(in, " ");
    while (token != NULL && argc < size - 1) {
        // remove newline at end
        char *newline = strchr(token, '\n');
        if (newline)
            *newline = 0;
        argv[argc] = strdup(token);
        argc += 1;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;
}

void PrintArgs(char **argv) {
    int i = 0;
    int j = 0;
    const char *bad[] = {
        "<",
        ">",
        "&",
    };

    while (argv[i]) {
        for (j = 0; j < 3; j++) {
            if (strcmp(argv[i], bad[j]) == 0) {
                return;
            }
        }
        printf("argv[%d] = '%s'\n", i, argv[i]);
        i++;
    }
}

void ReadCommand(char *line, struct Command *command) {
    int currToken = 0;
    const char delim[2] = "|";
    char *saveptr;
    char *token = strtok_r(line, delim, &saveptr);
    while (token != NULL && currToken < MAX_SUB_COMMANDS) {
        struct SubCommand *subcmd = malloc(sizeof(struct SubCommand));
        subcmd->line = strdup(token);
        ReadArgs(subcmd->line, subcmd->argv, MAX_ARGS);
        command->sub_commands[currToken] = *subcmd;
        currToken++;
        command->num_sub_commands += 1;
        token = strtok_r(NULL, delim, &saveptr);
    }
}

void ReadRedirectsAndBackground(struct Command *command) {
  command->stdin_redirect = NULL;
  command->stdout_redirect = NULL;
  command->background = 0;

  if (!command->num_sub_commands)
    return;

  struct SubCommand *sub_command = &command->sub_commands
  [command->num_sub_commands - 1];

  while (1) {
    int argc = 0;
    while (sub_command->argv[argc]) {
      argc++;
    }

    if (argc > 2 && !strcmp(sub_command->argv[argc - 2], ">")) {
      command->stdout_redirect = sub_command->argv[argc - 1];
      sub_command->argv[argc - 2] = NULL;
    } 
    else if (argc > 2 && !strcmp(sub_command->argv[argc - 2], "<")) {
      command->stdin_redirect = sub_command->argv[argc - 1];
      sub_command->argv[argc - 2] = NULL;
    }
    else if (argc > 1 && !strcmp(sub_command->argv[argc - 1], "&")) {
      command->background = 1;
      sub_command->argv[argc - 1] = NULL;
    } else {
      break;
    }
  }
}

void PrintCommand(struct Command *command) {
    int subCmdIdx = 0;
    while (subCmdIdx < command->num_sub_commands) {
        printf("Command %d:\n", subCmdIdx);
        PrintArgs(command->sub_commands[subCmdIdx].argv);
        printf("\n");
        subCmdIdx++;
    }
    if (command->stdin_redirect)
        printf("Redirect stdin: %s\n", command->stdin_redirect);
    if (command->stdout_redirect)
        printf("Redirect stdout: %s\n", command->stdout_redirect);
    printf("Background: %s\n", command->background == 1
                                   ? "yes"
                                   : "no");
}

void PrintCommandResult(struct Command *command) {

  if (command->background) {
    printf("[%d]\n", getpid());
  }

  printf("here 1\n");

  int subCmdIdx = 0;
  while (subCmdIdx < command->num_sub_commands) {

    pid_t pid;
    int status;
    char **argv = command->sub_commands[subCmdIdx].argv;
    char **argv2;
    int fds[2];
    int err;

    if (command->num_sub_commands > 1) {
      if (subCmdIdx < command->num_sub_commands - 1) {
        argv = command->sub_commands[command->num_sub_commands - subCmdIdx - 1].argv;
        argv2 = command->sub_commands[command->num_sub_commands - subCmdIdx - 2].argv;
        err = pipe(fds);
        if (err == -1) {
          perror("pipe");
          return; 
        }
      }
    }

    printf("here 2\n");

    pid = fork();
    if (pid == 0) {
      if (command->stdin_redirect) {
        // printf("Inside stdin_redirect\n");
        // open(command->stdin_redirect, O_RDONLY);
        int fd0;
        if ((fd0 = open(command->stdin_redirect, O_RDONLY, 0)) < 0) {
            perror("Couldn't open input file");
            exit(0);
        }           
        // dup2() copies content of fdo in input of preceeding file
        dup2(fd0, 0); // STDIN_FILENO here can be replaced by 0 
        close(fd0); // necessar
      }
      if (command->stdout_redirect) {
        // printf("Inside stdout_redirect\n");
        // open(command->stdout_redirect, O_WRONLY | O_CREAT | O_TRUNC);
        int fd1 ;
        if ((fd1 = creat(command->stdout_redirect, 0644)) < 0) {
            perror("Couldn't open the output file");
            exit(0);
        }           

        dup2(fd1, STDOUT_FILENO); // 1 here can be replaced by STDOUT_FILENO
        close(fd1);
      }

      if (command->num_sub_commands > 1) {
        close(fds[1]);
        close(0);
        dup(fds[0]);
      }

      if (execvp(argv[0], argv) == -1) {
        printf("%s: Command not found\n", argv[0]);
      }
      exit(EXIT_FAILURE);
    } else if (pid < 0) {
      // Error forking
      perror("lsh");
    } else {

      printf("here 3\n");
      
      if (command->num_sub_commands > 1) {
        if (subCmdIdx < command->num_sub_commands - 1) {
          close(fds[0]);
          close(1);
          dup(fds[1]);
          if (execvp(argv2[0], argv2) == -1) {
            printf("%s: Command not found\n", argv[0]);
          }
        }
      }

      printf("here 4\n");

      do {
        if (!command->background) {
          waitpid(pid, &status, WUNTRACED);
        }
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    printf("here 5\n");
    subCmdIdx++;

  }
}

int main(int argc, char **argv) {

      while (1) {
        struct Command *command = malloc(sizeof(struct Command));
        
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s$ ", cwd);
        } else {
            perror("getcwd() error");
            return 1;
        }
        char *buffer;
        size_t bufsize = 32;
        size_t characters;
        characters = getline(&buffer, &bufsize, stdin);
        ReadCommand(buffer, command);
        ReadRedirectsAndBackground(command);
        printf("\n");
        PrintCommand(command);
        printf("\n");
        PrintCommandResult(command);
      }
}
