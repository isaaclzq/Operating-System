#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include "tokenizer.h"
#include <sys/stat.h>
#include <fcntl.h>



/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t *fun;
  char *cmd;
  char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
  {cmd_help, "?", "show this help menu"},
  {cmd_exit, "exit", "exit the command shell"},
  {cmd_pwd, "pwd", "print current working directory"},
  {cmd_cd, "cd", "change directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(struct tokens *tokens) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(struct tokens *tokens) {
  exit(0);
}

int cmd_pwd(struct tokens *tokens) {
  char my_cwd[2048];
  getcwd(my_cwd, 2048);
  printf("%s\n", my_cwd);
  return 1;
}

int cmd_cd(struct tokens *tokens) {
  char *dir = tokens_get_token(tokens, 1);
  chdir(dir);
  return 1; 
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(int argc, char *argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens *tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      int status, my_pid, newfd, check_command, index = 0;
      char *name = "PATH";
      char *space = " ";
      char *redr_out = ">";
      char *redr_in = "<";
      int flag = 0;
      my_pid = fork();
      if (my_pid == 0) {
        char *parmList[tokens_get_length(tokens)+1];
        int x;
        for (x = 0; x < tokens_get_length(tokens); x++){
          if (strcmp(tokens_get_token(tokens, x), redr_out) == 0){
            flag = 1;
            index = x;
            break;
          } else if (strcmp(tokens_get_token(tokens, x), redr_in) == 0){
            flag = 2;
            index = x;
            break;
          }
          parmList[x] = tokens_get_token(tokens, x);
        }

        if (index != 0){
          if (flag == 1){
            for (; index < tokens_get_length(tokens); index++){
              parmList[index] = NULL;
            }
            if ((newfd = open(tokens_get_token(tokens, x+1), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0) {
              perror(tokens_get_token(tokens, x+1));
              exit(1);
            }     
            dup2(newfd, 1);
          } else {
            for (; index < tokens_get_length(tokens); index++){
              parmList[index] = NULL;
            }
            if ((newfd = open(tokens_get_token(tokens, x+1), O_RDWR)) < 0){
              exit(1);
            }
            dup2(newfd, STDIN_FILENO);
          }
        }
        parmList[tokens_get_length(tokens)] = NULL;
        check_command = execv(tokens_get_token(tokens, 0), parmList);  
        // relative path for command
        if (check_command == -1){
          char *path = getenv(name);
          char *delim = ":";
          char *new_delim = "/";
          char *addr = strtok(path, delim);
          struct stat st;
          while (addr != NULL){
            char abs_path[2048];
            memset(&abs_path[0], 0, sizeof(abs_path));
            strncpy(abs_path, addr, strlen(addr));
            strncat(abs_path, new_delim, strlen(new_delim));
            strncat(abs_path, tokens_get_token(tokens, 0), strlen(tokens_get_token(tokens, 0)));
            int result = stat(abs_path, &st);
            if (result == 0) {
              execv(abs_path, parmList);
              break;
            }
            addr = strtok(NULL, delim);
          }
        }  
      } 
      else if (my_pid < 0) {
        exit(EXIT_FAILURE);
        } 
      else {
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        waitpid(my_pid, &status, 0);
        }
      }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
