/**
*   Name:       Brianne O'Neil, Kyle Colantonio, David Loucks
*   Group:      C
*   Date:       17-NOV-2014
*   Project 2:  A Floppy Disk Shell Program
*   Goal:       Create a program that allows users to access a floppy disk locally mounted on a computer
*				and also execute any Linux command through a miniature shell.
*
*   UPDATED:    17-NOV-2014
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include "boot.h"
#include "process_management.h"

#define BUFFER_SIZE 1024

#define COMMAND_EXECUTE 0
#define COMMAND_QUIT 1
#define COMMAND_CLEAR 2
#define COMMAND_PATH_MOD 3
#define COMMAND_CWD 4

#define FLOP_MOUNT 5
#define FLOP_UMOUNT 6

/*
* Appends a directory to the end of the current PATH.
*/
void addPath(char *add) {
	char *path = getenv("PATH");
	strcat(path, ":");
	strcat(path, add);
	setenv("PATH", path, 1);
}

/*
* Deletes all instances of a directory from the PATH.
*/
void delPath(char *rem) {
	char *path = (char *)malloc((sizeof(char) * strlen(getenv("PATH"))) - (sizeof(char) * strlen(rem)));
	int i = 0; char *p;
	for (i = 0, p = strtok(getenv("PATH"), ":"); p != NULL; i++, p = strtok(NULL, ":")) {
		if (strcmp(p, rem) == 0) {
			continue; //Skip first
		} else {
			if (i != 0) {
				strcat(path, ":");
			}
			strcat(path, p);
		}
	}
	setenv("PATH", path, 1);
}

/*
* Prints the shell prompt.
*/
void printPrompt() {
	char cwd[BUFFER_SIZE];
	getcwd(cwd, sizeof(cwd));
	printf("[\033[1;33m%s\033[0m]shell\033[1;36m$\033[0m ", cwd);
}

/*
* Clears the shell screen.
*/
void clearScreen() {
	printf("\033[H\033[J");
}

/*
* Processes internal commands that can be handled later.
* Returns the corresponding integer value of the command.
*/
int process_internal_command(char* cmd){
	if (strstr(cmd, "exit") == cmd || strstr(cmd,"quit") == cmd) {
		return COMMAND_QUIT;
	}
	if (strstr(cmd, "clear") == cmd || strstr(cmd,"cls") ==  cmd) {
		return COMMAND_CLEAR;
	}
	if (strstr(cmd, "path") == cmd) {
		return COMMAND_PATH_MOD;
	}
	if (strstr(cmd, "cd") == cmd) {
		return COMMAND_CWD;
	}
    if (strstr(cmd, "fmount") == cmd) {
        return FLOP_MOUNT;
    }
    if (strstr(cmd, "fumount") == cmd) {
        return FLOP_UMOUNT;
    }
	return COMMAND_EXECUTE;
}

/*
* Main
*/
int main(int argc, char** argv){
	clearScreen();
    addPath(".");
	size_t buffer = BUFFER_SIZE;
	char* cmd = (char*)malloc(BUFFER_SIZE);
	int c = 1;
	while (c) {
		printPrompt();
		getline(&cmd, &buffer, stdin);
		char* ptr = cmd;
		while(*ptr==' ')
			ptr++;
		char* end = ptr + strlen(ptr) - 1;
		if(*end=='\n') {
			*end = '\0';
        }

        //Process internal commands
		switch (process_internal_command(ptr)) {
            /*
             * EXIT
             */
			case COMMAND_QUIT: {
				c = 0;
				break;
			}

            /*
             * PATH
             */
			case COMMAND_PATH_MOD: {
				ptr+=strlen("PATH");
				while(*ptr==' ')
					ptr++;
				if(*ptr=='+') { //path +
                    ptr++;
					addPath(ptr+1);
				} else if(*ptr=='-') { //path -
                    ptr++;
					delPath(ptr+1);
				}

                printf("%s\n", getenv("PATH"));
				break;
			}

            /*
             * CD
             */
			case COMMAND_CWD: {
				ptr+=strlen("CD");
				while(*ptr==' ')
					ptr++;
				if(*ptr=='\0') {
                    //Change to HOME directory if nothing is supplied
					chdir(getenv("HOME"));
				} else {
                    //Change to user defined directory
					if(chdir(ptr)<0) {
						printf("Error: No such directory exists [\"%s\"].\n",ptr);
					}
				}
				break;
			}

            /*
             * MOUNT
             */
            case FLOP_MOUNT: {

                ptr+=strlen("FMOUNT");
                while(*ptr==' ')
                    ptr++;

                //Check if mounted
                if (isMounted()) {
                    printf("    ERROR: floppy is already mounted.\n");
                    break;
                }

                //Check for correct usage
                if (*ptr == '\0') {
                    printf("Usage: fmount <image>\n");
                } else {
                    if (access(ptr, F_OK) != -1) {
                        mount(ptr);
                    } else {
                        printf("    `%s` does not exist. Please use a valid image.\n", ptr);
                    }
           	    }
	    		break;
            }

            /*
             * UNMOUNT
             */
            case FLOP_UMOUNT: {

                ptr+=strlen("FUMOUNT");
                while(*ptr==' ')
                    ptr++;

                //Check if mounted
                if (!isMounted()) {
                    printf("    ERROR: no floppy mounted.\n");
                    break;
                }

                //check for correct usage
                if (*ptr == '\0') {
                    unmount();
                } else {
                    printf("Usage: fumount\n");
                }
                break;
            }

            /*
             * CLEAR
             */
			case COMMAND_CLEAR: {
				clearScreen();
				break;
			}

            /*
             * DEFAULT: execute LINUX command
             */
            default: {
                process* proc = new_process(ptr);
                execute_process(proc);
                delete_process(proc);
                break;
            }
		}
	}
	return 0;
}
