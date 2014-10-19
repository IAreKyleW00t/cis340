/**
*   Name:       Brianne O'Neil, Kyle Colantonio, David Loucks
*   Group:      C
*   Date:       19-OCT-2014
*   Project 2:  A Floppy Disk Program
*   Goal:       Create a program that allows users to access a floppy disk locally mounted on a computer.
*
*   UPDATED:    18-OCT-2014
**/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "shell.h"
#include "boot.h"

#define TRUE        1
#define FALSE       0
#define BUFFER      128
#define MAX_ARGS    128

/*
*   Main Program
*/
int main(int argc, char *argv[]) {
    //Define local variables
    char *input;
    char **cmd_argv;
    int cmd_argc;

    //size_t object used in getline()
    size_t buffer;
    buffer = BUFFER;

    //Allocate variables to memory.
    cmd_argv = (char **)malloc(sizeof(char *) * MAX_ARGS);

    while (TRUE) {
        char *p;

        printPrompt(); //print "flop>"
			
		input = (char *)malloc(sizeof(char) * BUFFER);
        getline(&input, &buffer, stdin); //get input

        //Remove '\n' from end of input
        input[strcspn(input, "\n")] = '\0';

        //Split input into cmd_argv and cmd_argc
        int c;
        for (c = 0, p = strtok(input, " "); p != NULL; ++c, p = strtok(NULL, " ")) {
            cmd_argv[c] = (char *)malloc(strlen(p) + 1);
            strcpy(cmd_argv[c], p);
        }
        cmd_argc = c;

        //Check for '>' (output redirection)
        int file_out, con_out;
        fflush(stdout); //Safety precaution
        for (int i = 0; i < cmd_argc; i++) {
            if (strcmp(cmd_argv[i], ">") == 0) {
                //Output redirection
                file_out = open(cmd_argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                con_out = dup(STDOUT_FILENO); //Copy original STDOUT

                if (file_out) {
                    dup2(file_out, STDOUT_FILENO);
                    close(file_out);
                }
                break;
            }
        }

        //Check cmd_argv and act accordingly
        if (strcmp(cmd_argv[0], "exit") == 0)
            break;
        else if (strcmp(cmd_argv[0], "help") == 0)
            printHelp();
        else if (strcmp(cmd_argv[0], "fmount") == 0) {
            if (cmd_argc < 2) {
                printIncorrect();
                continue;
            }

            if (isMounted()) {
                printf("    `%s` is already mounted.\n", getFile());
                continue;
            }
			if (access(cmd_argv[1], F_OK) != -1)
				mount(cmd_argv[1]);
			else
				printf("	`%s` does not exist. Please use a valid filename.\n", cmd_argv[1]);
        } else if (strcmp(cmd_argv[0], "fumount") == 0) {
            if (!isMounted()) {
                printf("    ERROR: No floppy mounted.\n");
                continue;
            }

            unmount();
        } else if (strcmp(cmd_argv[0], "structure") == 0) {
            if (!isMounted()) {
                printf("    ERROR: No floppy mounted.\n");
                continue;
            }

            if (strcmp(cmd_argv[1], "-l") == 0)
                structure(1);
            else
                structure(0);
        } else if (strcmp(cmd_argv[0], "showsector") == 0) {
            if (!isMounted()) {
                printf("    ERROR: No floppy mounted.\n");
                continue;
            }

            if (cmd_argc < 2) {
                printIncorrect();
                continue;
            }

            showsector(atoi(cmd_argv[1]));
        } else if (strcmp(cmd_argv[0], "traverse") == 0) {
            if (!isMounted()) {
                printf("    ERROR: No floppy mounted.\n");
                continue;
            }

            if (strcmp(cmd_argv[1], "-l") == 0)
                traverse(1);
            else
                traverse(0);
        } else if (strcmp(cmd_argv[0], "showfat") == 0) {
            if (!isMounted()) {
                printf("    ERROR: No floppy mounted.\n");
                continue;
            }

            showfat();
        } else if (strcmp(cmd_argv[0], "showfile") == 0) {
            if (!isMounted()) {
                printf("    ERROR: No floppy mounted.\n");
                continue;
            }

            if (cmd_argc < 2) {
                printIncorrect();
                continue;
            }
            
            showfile(cmd_argv[1]);
        } else {
            //Invalid Command
            printInvalid();
        }

        fflush(stdout); //Safety precaution
        if (con_out) {
            dup2(con_out, STDOUT_FILENO);
            close(con_out);
        }
	
		//free input and cmd_argv
		free(input);		
		for (int i = 0; i < cmd_argc; i++)
			free(cmd_argv[i]);
    }

    //Exit successfully
    exit(EXIT_SUCCESS);
}
