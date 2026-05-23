#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
void readCommand(char * command){
     fgets(command, 1024, stdin);
     command[strlen(command) - 1] = '\0';
}
void parseCommand(char * argv[10], char command[1024], int *i, int * fd, int * amper, int * redirect, int * retid, int * status, char** outfile){
    *i = 0;
    char *token;
    token = strtok (command," ");
    while (token != NULL)
    {
        argv[*i] = token;
        token = strtok (NULL, " ");
        (*i)++;
    }
    argv[*i] = NULL;

    /* Is command empty */
    if (argv[0] == NULL) return;
    if (! strcmp(argv[*i - 1], "&")) {
        *amper = 1;
        argv[*i - 1] = NULL;
    }
    else 
        *amper = 0; 

    if (argv[0] && *i >= 2 && ! strcmp(argv[*i - 2], ">")) {
        *redirect = 1;
        argv[*i - 2] = NULL;
        outfile = argv[*i - 1];
        }
    else 
        *redirect = 0; 
}
int main() {
char command[1024];
char *token;
char *outfile;
//i - index of last argument (i itself is null, i-1 is the last)
//fd - file descriptor
//amper - ampersent sign at the end - meaning we run the command in the background
//redirect - instead of printing, we put it in the file listed after '>'
//retid - child process pid
//status - return status of the child process
int i, fd, amper, redirect, retid, status;
char *argv[10];
while (1)
{
    printf("hello: ");
    fflush(stdout);
    readCommand(command);
    parseCommand(argv, command, &i, &fd, &amper, &redirect, &retid, &status, &outfile);
    if (strcmp(argv[0], "quit") == 0) {
        break; // This successfully breaks the main loop!
    }
    if (fork() == 0) { 
        /* redirection of IO ? */
        if (redirect) {
            //0660 - premissions
            //0 - this is octal base 8
            //6 - premission level for the owner
            //second 6 - premission level for people in the owner group that are not the onwer
            //0 - premission level for other people
            //each number is an addition of these numbers: 1 - execute. 2 - write, 4 - read
            //in our case - 4 + 2 = 6 meaning read and write, but not execute.
            fd = creat(outfile, 0660); 
            close (STDOUT_FILENO) ; 
            dup(fd); 
            close(fd); 
            /* stdout is now redirected */
        } 
        //the first word is the program itself to run instead of the console.
        execvp(argv[0], argv);
    }
    /* parent continues here */
    if (amper == 0)
        retid = wait(&status);
}
}
