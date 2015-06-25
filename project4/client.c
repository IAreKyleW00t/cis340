/**
*   Name:       Kyle Colantonio, Rutledge Brandon, Robert Hosta
*   Group:      I
*   Date:       8-DEC-2014
*   Project 4:  A Remote Floppy Disk Shell
*   Goal:       Create a program that allows users to access a floppy disk remotely mounted on a computer through a socket. (Client)
*
*   UPDATED:    8-DEC-2014
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include "command.h"
#include "network.h"
#include "Fat12Boot.h"
#include "Fat12Entry.h"

#define BUFFER      1024
#define PORT        13115

#define INVALID_CMD -1
#define HELP        0
#define FMOUNT      1
#define FUMOUNT     2
#define STRUCTURE   3
#define TRAVERSE    4
#define SHOWFAT     5
#define SHOWSECTOR  6
#define SHOWFILE    7
#define EXIT        8

#define TRUE        1
#define FALSE       0

/* Global static variables */
static int uuid;
static int serv_uuid = -1;

Fat12Boot boot;
Fat12Entry *entry;

/*
 * Prints the terminal prompt to the screen
 */
void printPrompt() {
    printf("flop: ");
}

/*
 * Clears the terminal screen
 */
void clearScreen() {
    printf("\033[H\033[J");
}

/*
 * Prints help information.
 */
void printHelp() {
    printf("======== FLOP HELP =========\n");
    printf("  <required> - [optional]\n\n");
    printf("  fmount:\n");
    printf("    DESC:     Mounts a remote floppy image.\n");
    printf("    USAGE:    `fmount <server>`\n");
    printf("  fumount:\n");
    printf("    DESC:     Unmounts a remote floppy image.\n");
    printf("    USAGE:    `fumount`\n");
    printf("  structure:\n");
    printf("    DESC:     Lists the structure of the floppy disk.\n");
    printf("    USAGE:    `structure [-l]`\n");
    printf("  traverse:\n");
    printf("    DESC:     Lists the contents of the root directory.\n");
    printf("    USGE:     `traverse [-l]`\n");
    printf("  showfat:\n");
    printf("    DESC:     Show the content of the FAT tables.\n");
    printf("    USAGE:    `showfat`\n");
    printf("  showsector:\n");
    printf("    DESC:     Show the contents of a specified sector.\n");
    printf("    USAGE:    `showsector <number>`\n");
    printf("  showfile:\n");
    printf("    DESC:     Show the contents of a file.\n");
    printf("    USAGE:    `showfile <file>`\n");
    printf("  help:\n");
    printf("    DESC:     Displays all available commands.\n");
    printf("    USGAE:    `help`\n");
    printf("  exit:\n");
    printf("    DESC:     Exits the flop program.\n");
    printf("    USAGE:    `exit`\n");
}

/*
 * Counts number of words within a char *.
 */
int count_words(char *str) {
    int c = 1;
    char *ptr = str;

    while (*ptr != '\0') {
        if (*ptr == ' ')
            c++;

        if (*ptr == '<' || *ptr == '>') {
            c--;
            break;
        }
        ptr++;
    }

    return c;
}

/*
 * Command constructor
 * Used to create a new instance of a command by
 * malloc'ing and creating values based off the
 * given char * input.
 */
command *new_command(char *str) {
    char *c = str;
    int words = count_words(c);
    command *cmd = (command *)malloc(sizeof(command) + 1);
    cmd->name = NULL;
    cmd->argv = (char **)malloc(sizeof(char *) * words);
    cmd->output_file = NULL;
    cmd->input_file = NULL;
    cmd->argc = words;

    int i; char *p;
    for (i = 0, p = strtok(str, " "); p != NULL; i++, p = strtok(NULL, " ")) {
        if (strcmp(p, ">") == 0) {
            p = strtok(NULL, " ");
            cmd->output_file = (char *)malloc(sizeof(char) * strlen(p));
            strcpy(cmd->output_file, p);
            continue;
        }
        if (strcmp(p, "<") == 0) {
            p = strtok(NULL, " ");
            cmd->input_file = (char *)malloc(sizeof(char) * strlen(p));
            strcpy(cmd->input_file, p);
            continue;
        }
        cmd->argv[i] = (char *)malloc(sizeof(char) * strlen(p));
        strcpy(cmd->argv[i], p);
    }
    cmd->name = (char *)malloc(sizeof(char) * strlen(cmd->argv[0]));
    strcpy(cmd->name, cmd->argv[0]);

    return cmd;
}

/*
 * Command destructor
 * Used to delete a command and free
 * it from memory.
 */
void delete_command(command *cmd) {
    //Free everything within argv
    for (int i = 0; i < cmd->argc; i++) {
        cmd->argv[i] = NULL;
        free(cmd->argv[i]);
    }

    //Free argv
    if (cmd->argv)
        free(cmd->argv);

    //Free name
    if (cmd->name)
        free(cmd->name);

    //Free input_file
    if (cmd->input_file)
        free(cmd->input_file);

    //Free output_file
    if (cmd->output_file)
        free(cmd->output_file);
}

/*
 * Check if a given char * matches any
 * internal commands and returns the int
 * value of that command.
 */
int check_command(char *p) {
    if (strstr(p, "help") == p || strstr(p, "?") == p)
        return HELP;

    if (strstr(p, "fmount") == p)
        return FMOUNT;

    if (strstr(p, "fumount") == p)
        return FUMOUNT;

    if (strstr(p, "structure") == p)
        return STRUCTURE;

    if (strstr(p, "traverse") == p)
        return TRAVERSE;

    if (strstr(p, "showfat") == p)
        return SHOWFAT;

    if (strstr(p, "showsector") == p)
        return SHOWSECTOR;

    if (strstr(p, "showfile") == p)
        return SHOWFILE;

    if (strstr(p, "exit") == p || strstr(p, "quit") == p)
        return EXIT;

    return INVALID_CMD;
}

/*
 * Parse BOOT information from BOOT SECTOR
 */
void load_boot(unsigned char *bytes) {
    network_seek(11, SEEK_SET_N); //skip 11 bytes

    /* BYTES_PER_SECTOR (2 bytes) */
    network_read(&boot.BYTES_PER_SECTOR, 2, bytes);

    /* SECTORS_PER_CLUSTER (1 byte) */
    network_read(&boot.SECTORS_PER_CLUSTER, 1, bytes);

    /* RESERVED_SECTORS (2 bytes) */
    network_read(&boot.RESERVED_SECTORS, 2, bytes);

    /* NUM_OF_FATS (1 byte) */
    network_read(&boot.NUM_OF_FATS, 1, bytes);

    /* MAX_ROOT_DIRS (2 bytes) */
    network_read(&boot.MAX_ROOT_DIRS, 2, bytes);

    /* TOTAL_SECTORS (2 bytes) */
    network_read(&boot.TOTAL_SECTORS, 2, bytes);

    network_seek(1, SEEK_CUR_N); //skip 1 byte

    /* SECTORS_PER_FAT (2 bytes) */
    network_read(&boot.SECTORS_PER_FAT, 2, bytes);

    /* SECTORS_PER_TRACK (2 bytes) */
    network_read(&boot.SECTORS_PER_TRACK, 2, bytes);

    /* NUM_OF_HEADS (2 bytes) */
    network_read(&boot.NUM_OF_HEADS, 2, bytes);

    network_seek(11, SEEK_CUR_N); //skip 11 bytes

    /* VOLUME_ID (4 bytes) */
    network_read(&boot.VOLUME_ID, 4, bytes);

    /* VOLUME_LABEL (11 bytes) */
    network_read(&boot.VOLUME_LABEL, 11, bytes);
}

/*
 * Parse ENTRY information from ROOT_DIRECTORY
 */
void load_entry(int start, unsigned char *bytes) {
    /* Set reader to 0 */
    network_seek(0, SEEK_SET_N);

    /* Load data into entry */
    for (int i = start; i < (start + 16); i++) {
        /* FILENAME (8 bytes) */
        network_read(&entry[i].FILENAME, 8, bytes);

        /* EXT (3 bytes) */
        network_read(&entry[i].EXT, 3, bytes);

        /* ATTRIBUTES (1 byte) */
        network_read(&entry[i].ATTRIBUTES, 1, bytes);

        /* RESERVED (10 bytes) */
        network_read(&entry[i].RESERVED, 2, bytes);

        /* CREATION_TIME (2 bytes) */
        network_read(&entry[i].CREATION_TIME, 2, bytes);

        /* CREATION_DATE (2 bytes) */
        network_read(&entry[i].CREATION_DATE, 2, bytes);

        /* LAST_ACCESS_DATE (2 bytes) */
        network_read(&entry[i].LAST_ACCESS_DATE, 2, bytes);

        network_seek(2, SEEK_CUR_N); //skip 2 bytes

        /* MODIFY_TIME (2 bytes) */
        network_read(&entry[i].MODIFY_TIME, 2, bytes);

        /* MODIFY_DATE (2 bytes) */
        network_read(&entry[i].MODIFY_DATE, 2, bytes);

        /* START_CLUSTER (2 bytes) */
        network_read(&entry[i].START_CLUSTER, 2, bytes);

        /* FILE_SIZE (4 bytes) */
        network_read(&entry[i].FILE_SIZE, 4, bytes);
    }
}

/*
 * Clears out old BOOT and ENTRY variables
 */
void unload(Fat12Boot *boot, Fat12Entry **entry) {
    bzero((char *)boot, sizeof(boot));
    bzero((char *)*entry, sizeof(*entry));
}

/*
 * Compares two char*'s exactly
 */
int compare_char(char *str1, char *str2) {
    for (int i = 0; i < strlen(str1); i++) {
        if (str1[i] != str2[i])
            return 0;
    }
    return 1;
}

/*
 * Main program
 */
int main(int argc, char **argv) {
    int sock_fd, serv_len, n;
    struct sockaddr_in serv_addr;
    struct hostent *serv;

    struct {
        char message[BUFFER];
        int uuid;
    } msg_in, msg_out;

    /* Generate a random UUID */
    srand(time(NULL));
    uuid = rand();

    clearScreen();

    char *input;
    command *cmd;
    size_t buffer = BUFFER;
    while (TRUE) {
        /* Display prompt */
        printPrompt();

        /* malloc input to memory */
        input = (char *)malloc(sizeof(char) * BUFFER);

        /* Clear msg_in and msg_out */
        bzero((char *)msg_in.message, sizeof(msg_in.message));
        bzero((char *)msg_out.message, sizeof(msg_out.message));
        msg_in.uuid = -1;
        msg_out.uuid = uuid;

        /* Get input */
        getline(&input, &buffer, stdin);
        input[strcspn(input, "\n")] = '\0'; //Remove trailing '\n'

        /* Create command */
        cmd = new_command(input);

        /* Output redirection */
        int file_out, con_out;
        if (cmd->output_file) {
            file_out = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (file_out < 0) {
                perror("open");
                break;
            }
            con_out = dup(STDOUT_FILENO);
            dup2(file_out, STDOUT_FILENO);
            close(file_out);
        }

        /* Execute user command if possible */
        switch (check_command(cmd->name)) {
            case HELP: //help
                printHelp();
                break;
            case FMOUNT: //fmount
                if (serv_uuid != -1) {
                    printf("[ERROR] You are already connected to a server\n");
                    break;
                }
                if (cmd->argc != 2) {
                    printf("[ERROR] Usage: fmount <server>\n");
                    break;
                }

                printf("Connecting to `%s`... ", cmd->argv[1]);
                fflush(stdout); //Safety precaution

                /* Parse server */
                serv = gethostbyname(cmd->argv[1]);

                /* Open UDP socket */
                sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (sock_fd < 0) {
                    perror("socket");
                    break;
                }

                /* Write zero's to serv_addr */
                bzero((char *)&serv_addr, sizeof(serv_addr));

                /* Set up server */
                serv_addr.sin_family = AF_INET;
                bcopy(serv->h_addr, (char *)&serv_addr.sin_addr, serv->h_length);
                serv_addr.sin_port = htons(PORT);
                serv_len = sizeof(serv_addr);

                /* Send CONNECT command to server */
                sprintf(msg_out.message, "CONNECT");
                n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                if (n < 0) {
                    perror("sendto");
                    break;
                }

                /* Get response from server */
                n = recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, (struct sockaddr *)&serv_addr, (socklen_t *)&serv_len);
                if (n < 0) {
                    perror("recvfrom");
                    break;
                }

                if (strcmp(msg_in.message, "CONNECT") != 0) {
                    printf("[ERROR] Invalid response from server\n");
                    break;
                }

                /* Save server UUID */
                serv_uuid = msg_in.uuid;

                /* Clear msg_in and msg_out */
                bzero((char *)&msg_out.message, sizeof(msg_out.message));
                bzero((char *)&msg_in.message, sizeof(msg_in.message));
                msg_out.uuid = uuid;
                msg_in.uuid = -1;

                /* Get SECTOR 0 from server */
                sprintf(msg_out.message, "GETSECTOR 0");
                n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                if (n < 0) {
                    perror("sendto");
                    break;
                }

                /* Get response from server */
                n = recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, (struct sockaddr *)&serv_addr, (socklen_t *)&serv_len);
                if (n < 0) {
                    perror("recvfrom");
                    break;
                }

                /* Load bytes into Fat12Boot */
                load_boot((unsigned char *)msg_in.message);

                /* malloc Fat12Entry */
                entry = (Fat12Entry *)malloc(sizeof(Fat12Entry) * boot.MAX_ROOT_DIRS);

                /* Request all ROOT_DIRECTORY sector entries */
                for (int i = 0; i < 14; i++) {
                    /* Clear msg_in and msg_out */
                    bzero((char *)&msg_out.message, sizeof(msg_out.message));
                    bzero((char *)&msg_in.message, sizeof(msg_in.message));
                    msg_out.uuid = uuid;
                    msg_in.uuid = -1;

                    /* Get ROOT DIRECTORY sector from server */
                    sprintf(msg_out.message, "GETSECTOR %d", ((boot.NUM_OF_FATS * boot.SECTORS_PER_FAT) + 1) + i);
                    n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                    if (n < 0) {
                        perror("sendto");
                        break;
                    }

                    /* Get response from server */
                    n = recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, (struct sockaddr *)&serv_addr, (socklen_t *)&serv_len);
                    if (n < 0) {
                        perror("recvfrom");
                        break;
                    }

                    /* Load bytes into Fat12Entry */
                    load_entry(16 * i, (unsigned char *)msg_in.message);
                }

                printf("Connected!\n");
                break;

            case FUMOUNT: //fumount
                if (serv_uuid == -1) {
                    printf("[ERROR] You are not connected to any server\n");
                    break;
                }
                if (cmd->argc != 1) {
                    printf("[ERROR] Usage: fumount\n");
                    break;
                }

                sprintf(msg_out.message, "DISCONNECT");
                n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                if (n < 0) {
                    perror("sendto");
                    break;
                }

                /* Close and write zero's to server */
                close(sock_fd);
                bzero((char *)&serv_addr, sizeof(serv_addr));
                serv_uuid = -1;
                break;

            case STRUCTURE: //structure
                if (serv_uuid == -1) {
                    printf("[ERROR] You are not connected to any server\n");
                    break;
                }
                if (cmd->argc > 2) {
                    printf("[ERROR] Usage: structure [-l]\n");
                    break;
                } else if (cmd->argc == 2) {
                    if (strcmp(cmd->argv[1], "-l") != 0) {
                        printf("[ERROR] Usage: structure [-l]\n");
                        break;
                    }
                }

                printf("        number of FAT:                      %d\n", boot.NUM_OF_FATS);
                printf("        number of sectors used by FAT:      %d\n", boot.SECTORS_PER_FAT);
                printf("        number of sectors per cluster:      %d\n", boot.SECTORS_PER_CLUSTER);
                printf("        number of ROOT Entries:             %d\n", boot.MAX_ROOT_DIRS);
                printf("        number of bytes per sector          %d\n", boot.BYTES_PER_SECTOR);
                if (cmd->argc == 2) {
                    printf("        ---Sector #---      ---Sector Types---\n");
                    printf("             0                    BOOT\n");
                    for (int i = 0; i < boot.NUM_OF_FATS; i++)
                        printf("          %02d -- %02d                FAT%d\n", (boot.SECTORS_PER_FAT * i) + 1, boot.SECTORS_PER_FAT * (i + 1), i);

                    printf("          %02d -- %02d                ROOT DIRECTORY\n", boot.SECTORS_PER_FAT * boot.NUM_OF_FATS, (boot.MAX_ROOT_DIRS / 16) + (boot.SECTORS_PER_FAT * boot.NUM_OF_FATS));
                }
                break;

            case TRAVERSE: //traverse
                if (serv_uuid == -1) {
                    printf("[ERROR] You are not connected to any server\n");
                    break;
                }
                if (cmd->argc > 2) {
                    printf("[ERROR] Usage: traverse [-l]\n");
                    break;
                } else if (cmd->argc == 2) {
                    if (strcmp(cmd->argv[1], "-l") != 0) {
                        printf("[ERROR] Usage: traverse [-l]\n");
                        break;
                    }
                }

                if (cmd->argc == 2) {
                    printf("    *****************************\n");
                    printf("    ** FILE ATTRIBUTE NOTATION **\n");
                    printf("    **                         **\n");
                    printf("    ** R ------ READ ONLY FILE **\n");
                    printf("    ** S ------ SYSTEM FILE    **\n");
                    printf("    ** H ------ HIDDEN FILE    **\n");
                    printf("    ** A ------ ARCHIVE FILE   **\n");
                    printf("    *****************************\n");
                    printf("\n");

                    for (int i = 0; i < boot.MAX_ROOT_DIRS; i++) {
                        if (entry[i].FILENAME[0] != 0x00 && entry[i].START_CLUSTER != 0) {
                            char attr[6] = {'-', '-', '-', '-', '-'};
                            unsigned char a = entry[i].ATTRIBUTES[0];
                            if (a == 0x01)
                                attr[0] = 'R';
                            if (a == 0x02)
                                attr[1] = 'H';
                            if (a == 0x04)
                                attr[2] = 'S';
                            if (a == 0x20)
                                attr[5] = 'A';
                            if (a == 0x10) {
                                for (int j = 0; j < 6; j++)
                                    attr[j] = '-';
                            }

                            if (entry[i].ATTRIBUTES[0] == 0x10) {
                                printf("%.6s    %d %d       < DIR >      /%.8s                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILENAME, entry[i].START_CLUSTER);
                                printf("%.6s    %d %d       < DIR >      /%.8s/.                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILENAME, entry[i].START_CLUSTER);
                                printf("%.6s    %d %d       < DIR >      /%.8s/..                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILENAME, 0);
                            } else {
                                printf("%.6s    %d %d       %lu      /%.8s.%.3s                 %d\n", attr, entry[i].CREATION_DATE, entry[i].CREATION_TIME, entry[i].FILE_SIZE, entry[i].FILENAME, entry[i].EXT, entry[i].START_CLUSTER);
                            }
                        }
                    }
                } else {
                    for (int i = 0; i < boot.MAX_ROOT_DIRS; i++) {
                        if (entry[i].FILENAME[0] != 0x00 && entry[i].START_CLUSTER != 0) {
                            if (entry[i].ATTRIBUTES[0] == 0x10) {
                                printf("/%.8s                       < DIR >\n", entry[i].FILENAME);
                                printf("/%.8s/.                     < DIR >\n", entry[i].FILENAME);
                                printf("/%.8s/..                    < DIR >\n", entry[i].FILENAME);
                            } else {
                                printf("/%.8s.%.3s\n", entry[i].FILENAME, entry[i].EXT);
                            }
                        }
                    }
                }
                break;

            case SHOWFAT: //showfat
                if (serv_uuid == -1) {
                    printf("[ERROR] You are not connected to any server\n");
                    break;
                }
                if (cmd->argc != 1) {
                    printf("[ERROR] Usage: showfat\n");
                    break;
                }

                int sectors = (boot.NUM_OF_FATS * boot.SECTORS_PER_FAT);
                int count = 0;
                printf("        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f\n");

                /* Send GETSECTOR command to server */
                for (int i = 1; i <= sectors; i++) {
                    sprintf(msg_out.message, "GETSECTOR %d", i);
                    n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                    if (n < 0) {
                        perror("sendto");
                        break;
                    }

                    /* Get response from server */
                    n = recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, (struct sockaddr *)&serv_addr, (socklen_t *)&serv_len);
                    if (n < 0) {
                        perror("sendto");
                        break;
                    }

                    /* Check server UUID */
                    if (serv_uuid != msg_in.uuid) {
                        printf("[ERROR] Invalid server UUID - Connection rejected\n");
                        break;
                    }

                    for (int j = 0; j < boot.BYTES_PER_SECTOR; j++) {
                        if (count % 16 == 0 || count == 0) {
                            printf("\n");
                            printf("%4x", count);
                        }
                        printf("%5x", (unsigned char)msg_in.message[j]);
                        count++;
                    }
                }
                printf("\n");
                break;

            case SHOWSECTOR: //showsector
                if (serv_uuid == -1) {
                    printf("[ERROR] You are not connected to any server\n");
                    break;
                }
                if (cmd->argc != 2) {
                    printf("[ERROR] Usage: showsector <sector>\n");
                    break;
                }

                /* Parse sector */
                int sector = atoi(cmd->argv[1]);

                /* Send GETSECTOR command to server */
                sprintf(msg_out.message, "GETSECTOR %d", sector);
                n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                if (n < 0) {
                    perror("sendto");
                    break;
                }

                /* Get response from server */
                n = recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, (struct sockaddr *)&serv_addr, (socklen_t *)&serv_len);
                if (n < 0) {
                    perror("recvfrom");
                    break;
                }

                /* Check for valid server UUID */
                if (serv_uuid != msg_in.uuid) {
                    printf("[ERROR] Invalid server UUID - Connection rejected\n");
                    break;
                }

                /* Read response */
                printf("        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f\n");
                for (int i = 0; i < boot.BYTES_PER_SECTOR; i++) {
                    if (i % 16 == 0 || i == 0) {
                        printf("\n");
                        printf("%4x", i);
                    }
                    printf("%5x", (unsigned char)msg_in.message[i]);
                }
                printf("\n");
                break;

            case SHOWFILE: //showfile
                if (serv_uuid == -1) {
                    printf("[ERROR] You are not connected to a server\n");
                    break;
                }
                if (cmd->argc != 2) {
                    printf("[ERROR] Usage: showfile <filename>\n");
                    break;
                }

                char filename[12] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' , '\0'};

                int index = 0, k;
                for (k = 0; k < strlen(cmd->argv[1]) && k < 9; k++) {
                    if (cmd->argv[1][k] == '.') {
                        k++;
                        break;
                    }
                    filename[index] = cmd->argv[1][k];
                    index++;
                }

                index = 8;
                for (int j = k; j < strlen(cmd->argv[1]) && j < 12; j++) {
                    if (cmd->argv[1][j] == '\n' || cmd->argv[1][j] == '\0') {
                        break;
                    }
                    filename[index] = cmd->argv[1][j];
                    index++;
                }

                for (int i = 0; i < boot.MAX_ROOT_DIRS; i++) {
                    if (entry[i].FILENAME != 0x00 && entry[i].START_CLUSTER != 0) {
                        if (compare_char(filename, (char *)entry[i].FILENAME)) {
                            int sector = ((boot.MAX_ROOT_DIRS / 16) + (boot.SECTORS_PER_FAT * boot.NUM_OF_FATS) - 1) + entry[i].START_CLUSTER;
                            int size = (entry[i].FILE_SIZE / 512) + 1;
                            int count = 0;

                            printf("        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f\n");
                            for (int p = sector; p <= sector + size + 1; p++) {
                                sprintf(msg_out.message, "GETSECTOR %d", p);
                                n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                                if (n < 0) {
                                    perror("sendto");
                                    break;
                                }

                                n = recvfrom(sock_fd, &msg_in, sizeof(msg_in), 0, (struct sockaddr *)&serv_addr, (socklen_t *)&serv_len);
                                if (n < 0) {
                                    perror("recvfrom");
                                    break;
                                }

                                if (serv_uuid != msg_in.uuid) {
                                    printf("[ERROR] Invalid server UUID - Connection reject\n");
                                    break;
                                }

                                for (int j = 0; j < boot.BYTES_PER_SECTOR; j++) {
                                    if (j % 16 == 0 || j == 0) {
                                        printf("\n");
                                        printf("%4x", count++);
                                    }
                                    printf("%5x", (unsigned char)msg_in.message[j]);
                                }
                            }
                            printf("\n");
                        }
                    }
                }
                break;

            case EXIT: //exit
                free(input);
                delete_command(cmd);

                if (serv_uuid != -1) {
                    /* Send DISCONNECT command to server */
                    sprintf(msg_out.message, "DISCONNECT");
                    n = sendto(sock_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&serv_addr, (socklen_t)serv_len);
                    if (n < 0) {
                        perror("sendto");
                        exit(EXIT_FAILURE);
                    }

                    close(sock_fd);
                    bzero((char *)&serv_addr, sizeof(serv_addr));
                    serv_uuid = -1;
                }
                exit(EXIT_SUCCESS);

            case INVALID_CMD: //Invalid command (default)
                printf("[ERROR] %s: Command not found\n", cmd->name);
                break;
        }

        fflush(stdout); //Safety precaution

        /* Restore output */
        if (cmd->output_file) {
            dup2(con_out, STDOUT_FILENO);
            close(con_out);
        }

        /* Free memory */
        free(input);
        delete_command(cmd);
    }
}
