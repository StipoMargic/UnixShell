/***************************************************************************/ /**

  @file         main.c

  @author       Stephen Brennan

  @date         Thursday,  8 January 2015

  @brief        LSH (Libstephen SHell)

*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "tinydir.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd();
int lsh_touch(char **args);
int lsh_ls(char **args);
int lsh_mkdir(char **args);
int lsh_mv(char **args);
/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "pwd",
    "touch",
    "ls",
    "mkdir"
    "mv"};

int (*builtin_func[])(char **) = {&lsh_cd, &lsh_help, &lsh_exit, &lsh_pwd, &lsh_touch, &lsh_ls, &lsh_mkdir, &lsh_mv};

int lsh_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}
/*

  Builtin function implementations.
*/

/**
   @brief Bultin command: change directory.
   @param args List of args.  args[0] is "cd".  args[1] is the directory.
   @return Always returns 1, to continue executing.
 */
int lsh_cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_mkdir(char **args)
{
  if (args[1] == NULL)
    fprintf(stderr, "lsh: expected argument to \"mkdir\"\n");
  else
    mkdir(args[1], 0777);

  return 1;
}

int lsh_ls(char **args)
{
  tinydir_dir dir;
  if (tinydir_open(&dir, ".") == -1)
  {
    perror("Error opening file");
    goto bail;
  }

  while (dir.has_next)
  {
    tinydir_file file;
    if (tinydir_readfile(&dir, &file) == -1)
    {
      perror("Error getting file");
      goto bail;
    }

    printf("%s", file.name);
    if (file.is_dir)
    {
      printf("/");
    }
    printf("\n");

    if (tinydir_next(&dir) == -1)
    {
      perror("Error getting next file");
      goto bail;
    }
  }

bail:
  tinydir_close(&dir);
  return 1;
}

int lsh_mv(char **args)
{
  char *file = args[1];
  char *location = args[2];
  char newplace[50];

  {
    if (location[0] == '/') //check to see if input is a path
    {
      strcat(location, "/"); //if argument is a Full Path, prepare to mv to end of path.
      strcat(location, file);
      if (rename(file, location) == 0)
        printf("Successful\n");
      else
        printf("Error:\nDirectory not found\n");
    }
    else
    {
      DIR *isD;
      isD = opendir(location); // check if argument is a DIR in CWD

      if (isD == NULL)
      {
        if (rename(file, location) != 0)
          printf("Error: File not moved\n");
        else
          printf("Successful\n");
      }
      else
      {
        char *ptrL;
        ptrL = getcwd(newplace, 50); // get current working directory path
        strcat(newplace, "/");
        strcat(newplace, location); // attach mv location to path ptrL
        strcat(newplace, "/");
        strcat(newplace, file); // keep original file name
        if (rename(file, ptrL) != -1)
          printf("Successful\n");
        else
          printf("Error:\nDirectory not found in CWD\n");
        closedir(isD);
      }
    }
  }
  return 1;
}

int lsh_touch(char **args)
{
  FILE *fPtr;

  fPtr = fopen(args[1], "w");
  if (args[1] == NULL)
  {
    fprintf(stderr, "lsh: expected argument to \"touch\"\n");
  }
  else
  {
    if (fPtr == NULL)
    {
      printf("Unable to create file.\n");
    }
  }

  fclose(fPtr);

  return 1;
}

int lsh_pwd()
{
  char cwd[256];

  if (getcwd(cwd, sizeof(cwd)) == NULL)
    perror("getcwd() error");
  else
    printf("current working directory is: %s\n", cwd);

  return 1;
}
/**
   @brief Builtin command: print help.
   @param args List of args.  Not examined.
   @return Always returns 1, to continue executing.
 */
int lsh_help(char **args)
{
  int i;
  printf("Stephen Brennan's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
   @param args List of args.  Not examined.
   @return Always returns 0, to terminate execution.
 */
int lsh_exit(char **args)
{
  return 0;
}

/**
  @brief Launch a program and wait for it to terminate.
  @param args Null terminated list of arguments (including program).
  @return Always returns 1, to continue execution.
 */
int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // Child process
    if (execvp(args[0], args) == -1)
    {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    // Error forking
    perror("lsh");
  }
  else
  {
    // Parent process
    do
    {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/**
   @brief Execute shell built-in or launch program.
   @param args Null terminated list of arguments.
   @return 1 if the shell should continue running, 0 if it should terminate
 */
int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL)
  {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++)
  {
    if (strcmp(args[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(args);
    }
  }

  return lsh_launch(args);
}

/**
   @brief Read a line of input from stdin.
   @return The line from stdin.
 */
char *lsh_read_line(void)
{
#ifdef LSH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufsize = 0; // have getline allocate a buffer for us
  if (getline(&line, &bufsize, stdin) == -1)
  {
    if (feof(stdin))
    {
      exit(EXIT_SUCCESS); // We recieved an EOF
    }
    else
    {
      perror("lsh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define LSH_RL_BUFSIZE 1024
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer)
  {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    // Read a character
    c = getchar();

    if (c == EOF)
    {
      exit(EXIT_SUCCESS);
    }
    else if (c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize)
    {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token, **tokens_backup;

  if (!tokens)
  {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;

    if (position >= bufsize)
    {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens)
      {
        free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;
  int status;

  do
  {
    printf("> ");
    line = lsh_read_line();
    args = lsh_split_line(line);
    status = lsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   @brief Main entry point.
   @param argc Argument count.
   @param argv Argument vector.
   @return status code
 */
int main(int argc, char **argv)
{
  // Load config files, if any.

  // Run command loop.
  lsh_loop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}
