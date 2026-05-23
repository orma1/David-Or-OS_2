#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>
#include <signal.h>
char prompt_text[256] = "hello:";
void execute_pipeline(char *command) {
    char *commands[10];
    int num_cmds = 0;
    
    // 1. Split the command string by '|'
    char *token = strtok(command, "|");
    while (token != NULL) {
        commands[num_cmds++] = token;
        token = strtok(NULL, "|");
    }

    // 2. Create the pipes (we need one less pipe than the number of commands)
    int pipes[10][2]; 
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe failed");
            return;
        }
    }

    // 3. Fork a child for EACH command
    for (int i = 0; i < num_cmds; i++) {
        if (fork() == 0) {
            // Child Process!
            
            // If it's NOT the first command, get input from the PREVIOUS pipe's Read-End [0]
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // If it's NOT the last command, send output to the CURRENT pipe's Write-End [1]
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            // CRITICAL: Children must close ALL pipes before running the command, 
            // otherwise the programs will wait forever for input!
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Now we just parse and run THIS specific piece of the command (e.g. " sort -r ")
            char *argv[10];
            int idx = 0;
            char *cmd_token = strtok(commands[i], " ");
            while (cmd_token != NULL) {
                argv[idx++] = cmd_token;
                cmd_token = strtok(NULL, " ");
            }
            argv[idx] = NULL;
            if (idx >= 3) {
                if (strcmp(argv[idx - 2], ">") == 0) {
                    int out_fd = open(argv[idx - 1], O_WRONLY | O_CREAT | O_TRUNC, 0660);
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                    argv[idx - 2] = NULL; // Cut off the > and filename so execvp doesn't see them
                } 
                else if (strcmp(argv[idx - 2], ">>") == 0) {
                    int out_fd = open(argv[idx - 1], O_WRONLY | O_CREAT | O_APPEND, 0660);
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                    argv[idx - 2] = NULL;
                }
            }
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(1);
        }
    }

    // 4. Parent Process: Close all pipes
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // 5. Parent Process: Wait for all children to finish
    for (int i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}
void handle_sigint(int sig) {
    printf("\nYou typed Control-C!\n%s", prompt_text);
    fflush(stdout);
}
void readCommand(char * command){
    if (fgets(command, 1024, stdin) == NULL) {
        clearerr(stdin);
        command[0] = '\0'; // Make it an empty string so the shell just loops
        return;
    }
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

    *redirect = 0;
    // We check if we have at least 3 arguments (e.g., "cat", "<", "file")
    if (*i >= 3) {
        if (!strcmp(argv[*i - 2], ">")) {
            *redirect = 1;
        } else if (!strcmp(argv[*i - 2], ">>")) {
            *redirect = 2;
        } else if (!strcmp(argv[*i - 2], "2>")) {
            *redirect = 3;
        } else if (!strcmp(argv[*i - 2], "<")) {
            *redirect = 4;
        }

        // If we found ANY redirect symbol
        if (*redirect != 0) {
            *outfile = argv[*i - 1]; // Save the file name
            argv[*i - 2] = NULL;     // Cut the argv array before the symbol
        }
    }
}
int main() {
int signal_done = 0;
signal(SIGINT, handle_sigint);
char command[1024];
char last_command[1024] = "";
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
     printf("%s ", prompt_text);
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
    // Check if the command contains a Pipe!
    if (strchr(command, '|') != NULL) {
        execute_pipeline(command);
        continue; // We are done! Skip the rest of the loop and prompt again.
    }

    // If there is no pipe, continue normally:
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
            // Make sure the user actually typed a folder name after 'cd'
            if (argv[1] != NULL) {
                // Try to change the directory
                if (chdir(argv[1]) != 0) {
                    // If chdir returns anything other than 0, it failed.
                    // perror automatically prints the exact reason (e.g., "No such file")
                    perror("cd failed"); 
                }
            } else {
                printf("cd: missing argument\n");
            }
            
            // CRITICAL: Jump back to the top of the loop!
            continue; 
        }
   if (fork() == 0) { 
        if (redirect) {
            if (redirect == 1) { // > (Overwrite STDOUT)
                fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
                dup2(fd, STDOUT_FILENO);
            } 
            else if (redirect == 2) { // >> (Append STDOUT)
                fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0660);
                dup2(fd, STDOUT_FILENO);
            } 
            else if (redirect == 3) { // 2> (Overwrite STDERR)
                fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0660);
                dup2(fd, STDERR_FILENO); // Notice we replace STDERR, not STDOUT!
            } 
            else if (redirect == 4) { // < (Read STDIN)
                fd = open(outfile, O_RDONLY);
                if (fd < 0) { // If the file does not exist
                    perror("No such file");
                    exit(1);  // Kill the child process so it doesn't run the command
                }
                dup2(fd, STDIN_FILENO); // Replace the keyboard with the file
            }
            
            // Clean up: close the original fd now that it has been copied
            if (fd >= 0) close(fd);
        }
        //the first word is the program itself to run instead of the console.
        execvp(argv[0], argv);
        perror("Command failed"); // This only prints if execvp fails (e.g. command not found)
        exit(1); 
    }
    
    /* parent continues here */
    if (amper == 0)
        retid = wait(&status);
}
}
