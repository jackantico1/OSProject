#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
    if (command->num_sub_commands == 0)
        return;
    int lastSubCmdIdx = command->num_sub_commands - 1;
    struct SubCommand *lastSubCmd = &(command->sub_commands[lastSubCmdIdx]);

    int i = MAX_ARGS - 1;
    int isBgDone = 0;
    int isRedirectInDone = 0;
    int isRedirectOutDone = 0;

    char *prevArg;
    while (i >= 0) {
        if (lastSubCmd->argv[i]) {
            if (isBgDone == 0) {
                command->background = strcmp(lastSubCmd->argv[i], "&") == 0 ? 1 : 0;
                isBgDone = 1;
            } else if (isRedirectOutDone == 0 && strcmp(lastSubCmd->argv[i], ">") == 0) {
                if (prevArg) command->stdout_redirect = strdup(prevArg);
                isRedirectOutDone = 1;
            } else if (isRedirectInDone == 0 && strcmp(lastSubCmd->argv[i], "<") == 0) {
                if (prevArg) command->stdin_redirect = strdup(prevArg);
                isRedirectInDone = 1;
            }
            prevArg = lastSubCmd->argv[i];  // track the previous argument
        }
        i -= 1;
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

int PrintArgsResult(char **argv) {
    int i = 0;
    int j = 0;
    int k = 0;
    int isValidCommand = 0;
    const char *bad[] = {
        "<",
        ">",
        "&",
    };
    char validCommands[3][10] = {
      "ls",
      "wc",
      "blah"
    };

    while (argv[i]) {

        for (j = 0; j < 3; j++) {
            if (strcmp(argv[i], bad[j]) == 0) {
                return 0;
            }
        }

        for (k = 0; k < 3; k++) {
          if (strcmp(argv[i], bad[k]) == 0) {
                isValidCommand = 1;
            }
        }

        if (isValidCommand != 1) {
          printf("%s: Command not found\n", argv[i]);
          return 0;
        }
        i++;
    }
    return 1;
}

void PrintCommandResult(struct Command *command) {

  int subCmdIdx = 0;
  while (subCmdIdx < command->num_sub_commands) {
      int isValid = PrintArgsResult(command->sub_commands[subCmdIdx].argv);
      if (isValid == 0) {
        return;
      }
      subCmdIdx++;
  }

  if (command->background == 1) {
      printf("[%d]\n", getpid());
  }
}

int main(int argc, char **argv) {

      while (1) {
        struct Command *command = malloc(sizeof(struct Command));
        printf("$ ");
        char *buffer;
        size_t bufsize = 32;
        size_t characters;
        characters = getline(&buffer, &bufsize, stdin);
        ReadCommand(buffer, command);
        ReadRedirectsAndBackground(command);
        PrintCommandResult(command);
        PrintCommand(command);
      }
}