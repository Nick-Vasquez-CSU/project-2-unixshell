
/*
-------------------------------------------------------------------------------------------------------
| Project #2 CPSC 351                                                                                 |
| by: Nicolas Vasquez                                                                                 |
|                                                                                                     |
-------------------------------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void parseCmd(char* inp, char** parsedArg){
  int i;

  for (i = 0; i < BUFSIZ; i++) {
    parsedArg[i] = strsep(&inp, " ");

    if (parsedArg[i] == NULL)
      break;
    if (strlen(parsedArg[i]) == 0)
      i--;
  }
}

int checkInput(char* inp, char** parsedArg, char** pipedArg){
  char* pipedInput[2];
  int pipeVal = 0;

  pipeVal = findPipe_IO_ampersand(inp, pipedInput);

  if (pipeVal == 1){
    parseCmd(pipedInput[0], parsedArg);
    parseCmd(pipedInput[1], pipedArg);
  }
  else{
    parseCmd(inp, parsedArg);
  }

  return 1+pipeVal;
}

int findPipe_IO_ampersand(char* inp, char** pipedInput){ //Checks input to see if there is a pipe
  int i;
  for(i = 0; i < 2; i++){
    pipedInput[i] = strsep(&inp, "|");
    if(pipedInput[i] == NULL)
      break;
  }

  if(pipedInput[1] == NULL) //No pipe found within command
    return 0;
  else{
    return 1;
  }
}

void system_execute(char* inp, char** parsedArg){ //Non-piped system command
  pid_t pid = fork();

  if (pid == -1){
    printf("\nFailed forking child");
    return;
  }
  else if (pid == 0){
    if (execvp(parsedArg[0], parsedArg) < 0)
      printf("\nCould not compute command");
    if (strstr(inp, "&"))
      strsep(&inp, "&");
      wait(NULL);
      wait(NULL);
  }
  else{
    wait(NULL);
    return;
  }
}

void piped_execute(char* inp, char** parsedArg, char** pArgPiped){ //Piped system command
  
  int pipeArr[2]; //0 = Read, 1 = Write
  pid_t pid1, pid2; //Child 1 and Child 2

  if (pipe(pipeArr) < 0){ //If pipe array is 0
    printf("\nPipe failed intialization"); 
  }

  pid1 = fork();
  if (pid1 < 0){
    printf("\nCannot fork pid1");
    return;
  }

  if (pid1 == 0){//Child 1 writes
    close(pipeArr[0]);
    dup2(pipeArr[1], STDOUT_FILENO);
    close(pipeArr[1]);

    if(execvp(parsedArg[0], parsedArg) < 0){
      printf("\nCommand failed execution");
      exit(0);
    }
  }
  else{
    pid2 = fork();
    if (pid2 < 0){
      printf("\nCannot fork pid2");
      return;
    }

    if(pid2 == 0){//Child 2 reads
      close(pipeArr[1]);
      dup2(pipeArr[0], STDIN_FILENO);
      close(pipeArr[0]);
      if (strstr(inp, "&"))
        strsep(&inp, "&");
        wait(NULL);
        wait(NULL);
      if(execvp(pArgPiped[0], pArgPiped) < 0){
        printf("\nCommand failed execution");
        exit(0);
      }
    }
    else{
      wait(NULL);
      wait(NULL);
    }
  }
}

int main(int argc, const char * argv[]) {  
  char input[BUFSIZ], *pArg[BUFSIZ];
  char* pArgPiped[BUFSIZ]; 
  char last_command[BUFSIZ];
  
  int commandFlag = 0; //Used to determine whether the command given requires a pipe or not
  
  memset(input, 0, BUFSIZ * sizeof(char));
  memset(input, 0, BUFSIZ * sizeof(char));
  bool finished = false;
  
  while (!finished) {
    printf("osh> ");
    fflush(stdout);

    if ((fgets(input, BUFSIZ, stdin)) == NULL) {   // or gets(input, BUFSIZ);
      fprintf(stderr, "no command entered\n");
      exit(1);
    }
    input[strlen(input) - 1] = '\0';          // wipe out newline at end of string
    printf("input was: \n'%s'\n", input);


    //--EXIT (exit()) Command--//
    if (strncmp(input, "exit", 4) == 0) {   // only compare first 4 letters
      finished = true;
    } 
    //End of EXIT Command--//

    else { 
      printf("You entered: %s\n", input);   // you will call fork/exec
      
      //--HISTORY (!!) Command--//
      if (strncmp(input, "!!", 2) == 0) {
      
        if (strlen(last_command) == 0) {
          fprintf(stderr, "no last command to execute\n");
        }
        else{
          printf("last command was: %s\n", last_command);
          strcpy(input, last_command);
        }
      } 
      else{
        strcpy(last_command, input);
      }
      //--End of HISTORY Command--//

      
      //Begin forking
      commandFlag = checkInput(input, pArg, pArgPiped);


      if(strncmp(input, ">", 1)){//Sends to output file

        int k = open(pArg[0], O_TRUNC | O_CREAT | O_RDWR);
        dup2(k, STDOUT_FILENO);
      }
      else if(strncmp(input, "<", 1)){//Sends to input file
        int k = open(pArg[0], O_RDONLY);
        memset(input, 0, BUFSIZ * sizeof(char));
        read(k, input, BUFSIZ * sizeof(char));
        memset(pArg[0], 0, BUFSIZ * sizeof(char));
      }

      if (commandFlag == 1){
        system_execute(input, pArg);
      }
      if (commandFlag == 2){
        piped_execute(input, pArg, pArgPiped);
      }
      printf("\n");
    }
  }
  
  printf("osh exited\n");
  printf("program finished\n");
  
  return 0;
}

//Did not end up needing
/*
void parse(char* s) {
  const char break_chars[] = " \t";
  char* p;
  p = strtok(s, break_chars);
  while (p != NULL) {
    printf("token was: %s\n", p);
    p = strtok(NULL, break_chars);
 }
}*/