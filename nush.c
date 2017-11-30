/*
 * Elements of this code (specifically piping/forking strategy) 
 * were inspired by code shown in lecture, author Nat Tuck.
 */
#include "svec.h"
#include "tokenize.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

bool execute(svec* tokens);

void
check_rv(int rv) {
    if (rv == -1) {
      perror("fail");
      exit(1);
    }
}



void
ex_pipe(svec* sv, int i) {
    // i is index of pipe
    // split tokens on left/right side of pipe   
    svec* right_args = make_svec();
    char* args_left[i];
    memset(args_left, 0, i*sizeof(args_left[0]));
    for (int j = 0; j < get_length(sv); ++j) {
      if (j < i) {
        args_left[j] = get(sv, j);
      }
      else if (j > i) {
        svec_push_back(right_args, (get(sv, j)));
      }
      else {
        args_left[j] = 0;
      }
    }
    // set up pipe and child PIDs
    int rv;
    int pipe_fds[2];
    rv = pipe(pipe_fds);
    check_rv(rv);
    int p_read = pipe_fds[0];
    int p_write = pipe_fds[1];
    int cpid_left;
    int cpid_right = 0;
    
    if (cpid_left = fork()) {
      // parent (everything on right side of pipe)
      close(p_write);
      int status;
      waitpid(cpid_left, &status, 0);
      close(p_write);
      close(0);
      rv = dup2(p_read, 0);
      check_rv(rv);
      // use execute instead of execvp to handle multiple ops
      execute(right_args);
    }
    else {
      // left child
      close(p_read);
      close(1);
      rv = dup2(p_write, 1);
      check_rv(rv);
      // use execvp so we can wait for this process to finish
      execvp(args_left[0], args_left);      
      perror("returned from execvp");
    }
}

void
ex_bool(svec* sv, int i, bool a_or_o)
{
    // i is index of && or ||
    // a_or_o is true for &&, false for ||

    // split tokens on left/right side of pipe
    svec* right_args = make_svec();
    char* args_left[i];
    memset(args_left, 0, i*sizeof(args_left[0]));
    for (int j = 0; j < get_length(sv); ++j) {
      if (j < i) {
        args_left[j] = get(sv, j);
      }
      else if (j > i) {
        svec_push_back(right_args, get(sv, j));
      }
      else {
        args_left[j] = 0;
      }
    }


    int cpid;
    if (cpid = fork()) {
      //parent (right side of bool op)
      //wait for left side, if successful run right side.
      int status;
      waitpid(cpid, &status, 0);
      //&&
      if (status == 0 && a_or_o == true) {
          if (strcmp(get(right_args, 0), "exit") == 0) {
            exit(0);
          }
          execute(right_args);
      }
      //||
      else if (status != 0 && a_or_o == false) {
        if (strcmp(get(right_args, 0), "exit") == 0) {
          exit(0);
        }
        execute(right_args);
      }
    }
    else {
      execvp(args_left[0], args_left);
      perror("returned from execvp");
    }
}

void
ex_bkgd(svec* sv, int i)
{
  // i is index of &
  if (i == 0) {
    printf("Misformatted background command");
    return;
  }
  // turn svec into arg array  
  char* args[i];
  for (int j = 0; j < i; ++j) {
    args[j] = get(sv, j);
  }
  args[i] = 0;
  
  int cpid;
  if (cpid = fork()) {    
  }
  else {
    execvp(args[0], args);
    perror("returned from execvp");
  }
}

void
ex_semi(svec* sv, int i) 
{
    // i is index of ;
    // split tokens on left/right side of pipe
    svec* left_args = make_svec();
    svec* right_args = make_svec(); 
    for (int j = 0; j < get_length(sv); ++j) {
      if (j < i) {
        svec_push_back(left_args, get(sv, j));
      }
      else if (j > i) {
        svec_push_back(right_args, get(sv, j));
      }
    }
    execute(left_args);
    execute(right_args);
}

void 
ex_input(svec* sv, int i)
{ 
    // i is index of <
    // split tokens on left/right side of <
    char* args_left[i];
    memset(args_left, 0, i*sizeof(args_left[0]));
    char* args_right[get_length(sv) - i];
    for (int j = 0; j < get_length(sv); ++j) {
      if (j < i) {
        args_left[j] = get(sv, j);
      }
      else if (j > i) {
        args_right[j - 1 - i] = get(sv, j);
      }
      else {
        args_left[j] = 0;
      }
    }
    args_right[get_length(sv) - i - 1] = 0;
    
    int cpid;

    if (cpid = fork()) {
      //parent
      int status;
      waitpid(cpid, &status, 0);
    }
    else {
      //child
      int fd = open(args_right[0], O_RDONLY);
      dup2(fd, 0);
      close(fd);
      execvp(args_left[0], args_left); 
      perror("returned from execvp");
    }
    
}

void
ex_output(svec* sv, int i)
{
    // i is index of >
    // split tokens on left/right side of >
    char* args_left[i];
    memset(args_left, 0, i*sizeof(args_left[0]));
    char* args_right[get_length(sv) - i];
    for (int j = 0; j < get_length(sv); ++j) {
      if (j < i) {
        args_left[j] = get(sv, j);
      }
      else if (j > i) {
        args_right[j - 1 - i] = get(sv, j);
      }
      else {
        args_left[j] = 0;
      }
    }
    args_right[get_length(sv) - i - 1] = 0;
    
    int cpid;

    if (cpid = fork()) {
      //parent
      int status;
      waitpid(cpid, &status, 0);
    }
    else {
      //child
      int fd = open(args_right[0], O_CREAT | O_TRUNC | O_WRONLY, 0666);      
      dup2(fd, 1);
      close(fd);
      execvp(args_left[0], args_left); 
      perror("returned from execvp");
    }
}

bool
execute(svec* tokens)
{
    if (get_length(tokens) == 0) {    
      free_svec(tokens);
      return false;
    }

    if (strcmp("exit", get(tokens, 0)) == 0 || feof(stdin)) {
      fclose(stdin);
      free_svec(tokens);
      exit(0);
      return true;
    }

    if (strcmp("cd", get(tokens, 0)) == 0) {
      chdir(get(tokens, 1));
      free_svec(tokens);
      return false;
    }

    for (int i = 0; i < get_length(tokens); ++i) {
      //check for semicolon
      if (strcmp(";", get(tokens, i)) == 0) {
        ex_semi(tokens, i);
        free_svec(tokens);
        return false;
      }
    }

    for (int i = 0; i < get_length(tokens); ++i) {
      // check for pipe
      if (strcmp("|", get(tokens, i)) == 0)  {
        ex_pipe(tokens, i);
        free_svec(tokens);
        return false;
      } 

      // check for logical operators
      else if (strcmp("&&", get(tokens, i)) == 0) {
        ex_bool(tokens, i, true);
        free_svec(tokens);
        return false;
      }
      else if (strcmp("||", get(tokens, i)) == 0) {
        ex_bool(tokens, i, false);        
        free_svec(tokens);
        return false;
      } 

      //check for background
      else if (strcmp("&", get(tokens, i)) == 0) {
        ex_bkgd(tokens, i);
        free_svec(tokens);
        return false;
      }
      
      //check for semicolon
      else if (strcmp(";", get(tokens, i)) == 0) {
        ex_semi(tokens, i);
        free_svec(tokens);
        return false;
      }

      //check for redirect
      else if (strcmp("<", get(tokens, i)) == 0) {
        ex_input(tokens, i);
        free_svec(tokens);
        return false;
      }
      else if (strcmp(">", get(tokens, i)) == 0) {
        ex_output(tokens, i);
        free_svec(tokens);
        return false;
      }
    }

    char* cmd = get(tokens, 0);
    int cpid;
    if ((cpid = fork())) {
        // parent process
        // Child may still be running until we wait.
        int status;
        waitpid(cpid, &status, 0);
        free_svec(tokens);
    }
    else {
        // The argv array for the child.
        // Terminated by a null pointer.
        char *args[get_length(tokens) + 1];
        for (int i = 0; i < get_length(tokens); ++i) {
          args[i] = get(tokens, i);
        }
        args[get_length(tokens)] = NULL;
        execvp(cmd, args);
        // shouldn't be able to reach here, but to be safe let's exit
        perror("returned from execvp");
        exit(1);
    }
    return false;
}

int
main(int argc, char* argv[])
{

  char cmd[256];
  char last_cmd[256];
  last_cmd[0] = '\0';

//script
  if (argc > 1) {
    FILE* script = fopen(argv[1], "r");
    if (script == NULL) {
      perror("Read error.");
      return 1;
    }
    while (!feof(script)) {
      if (fgets(cmd, 256, script) != NULL) {
        if (strcmp(cmd, last_cmd) == 0) {
          fclose(script);
          return 0;
        }
        svec* tokens = tokenize(cmd);
        execute(tokens);
        strcpy(last_cmd, cmd);
      }
      else {
        fclose(script);
        return 0;
      }
    }
    fclose(script);
    return 0;
  }


//no script
  while(!feof(stdin)) {
        printf("nush$ ");
        fflush(stdout);
        fgets(cmd, 256, stdin);
        // handle ^D case
        if (feof(stdin)) {
          printf("\n");
          return 0;
        }
    svec* tokens = tokenize(cmd);
    // execute returns true if exit signaled
    if (execute(tokens)) {
      return 0;
    }
  }   
  return 0;
}

