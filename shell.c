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
        *outfile = argv[*i - 1];
        }
    else 
        *redirect = 0; 
}
int main() {
char command[1024];
char last_command[1024] = "";
char prompt_text[256] = "hello: ";
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
     printf("%s", prompt_text);
    fflush(stdout);
    readCommand(command);
    if (strcmp(command, "!!") == 0) {
        if (last_command[0] == '\0') {
            continue; // No history yet
        }
        // Replace the current command with the last one
        strcpy(command, last_command);
        printf("%s\n", command); // Print it so the user sees what is running
    } else {
        // If it wasn't !!, save whatever they typed as the new history
        strcpy(last_command, command); 
    }
    parseCommand(argv, command, &i, &fd, &amper, &redirect, &retid, &status, &outfile);
     /* 5. Quit command */
    if (strcmp(argv[0], "quit") == 0) {
        exit(0); // will finish the program
    }
    /* 1. Prompt command */
    if (strcmp(argv[0], "prompt") == 0) {
        // we make sure that the promt is valid: the second word is '=' and there is the 3 word
        if (argv[1] != NULL && strcmp(argv[1], "=") == 0 && argv[2] != NULL) {
            strcpy(prompt_text, argv[2]); //copies the new word
            strcat(prompt_text, ": ");    // adds 'collum' and 'space' at the and
        }
        continue; // goes back to the begining without doing fork
    }
    /* 2. Status command */
        if (strcmp(argv[0], "status") == 0) {
            // WEXITSTATUS 
            printf("%d\n", WEXITSTATUS(status));
            continue;
        }

        /* 3. cd command */
        if (strcmp(argv[0], "cd") == 0) {
            if (argv[1] != NULL) {
                if (chdir(argv[1]) != 0) {
                    perror("cd failed"); // prints error if it doesnt exicts
                }
            }
            continue;
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
