/**
*   Name:       Kyle Colantonio, Rutledge Brandon, Robert Hosta
*   Group:      I
*   Date:       8-DEC-2014
*   Project 4:  A Remote Floppy Disk Shell
*   Goal:       Create a program that allows users to access a floppy disk remotely mounted on a computer through a socket. (Server)
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

#define BUFFER      1024
#define LISTEN_PORT 13115
#define BACKLOG     5

#define INVALID_CMD -1
#define CONNECT     0
#define DISCONNECT  1
#define GETSECTOR   2

#define TRUE        1
#define FALSE       0

FILE *floppy = NULL;

static int uuid;
static int client_uuid = -1;

/*
 * Check command
 */
int check_command(char *p) {
    if (strstr(p, "CONNECT") == p) {
        return CONNECT;
    }
    if (strstr(p, "DISCONNECT") == p) {
        return DISCONNECT;
    }
    if (strstr(p, "GETSECTOR") == p) {
        return GETSECTOR;
    }
    return INVALID_CMD;
}

/*
 * Main
 */
int main(int argc, char **argv) {
    int socket_fd, client_len, n;
    struct sockaddr_in server_addr, client_addr;

    struct {
        char message[BUFFER];
        int uuid;
    } msg_in, msg_out;

    /* Generates a unique, random integer for the server */
    srand(time(NULL));
    uuid = rand();

    /* Check for valid arguments */
    if (argc != 2) {
        printf("Usage: %s <image>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Open floppy image as READ_ONLY */
    floppy = fopen(argv[1], "r");
    if (floppy < 0) { //fopen() failed
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Open UDP Socket */
    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) { //socket() failed
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Write zero's to server_addr */
    bzero((char *)&server_addr, sizeof(server_addr));

    /* Set up server */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(LISTEN_PORT);

    /* Bind server */
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        //bind() fails to open if something is already using the ip:port
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* Listen for connections with (5) BACKLOGS */
    listen(socket_fd, BACKLOG);

    /* Welcome message */
    printf("  FLOPPY SERVER STARTED\n");
    printf("-------------------------\n");
    printf("Listening on %s:%d\n\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    fflush(stdout); //Safety precaution

    //Constantly listen for new connections
    while (TRUE) {
        /* Get sizeof client_addr */
        client_len = sizeof(client_addr);

        /* Clear msg structure */
        bzero((char *)msg_in.message, BUFFER);
        bzero((char *)msg_out.message, BUFFER);
        msg_in.uuid = -1;
        msg_out.uuid = uuid;

        /*
         * Receive data from the client
         * We need to use recvfrom() because
         * we are using a UDP socket.
         */
        n = recvfrom(socket_fd, &msg_in, sizeof(msg_in), 0, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
        if (n < 0) { //recvfrom() failed
            perror("socket:recvfrom");
            continue;
        }
        printf("%s:%d >>> %s\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port), msg_in.message);
        fflush(stdout); //Safety precaution

        /* Check client input */
        switch (check_command(msg_in.message)) {
        /* INVALID COMMAND */
        case INVALID_CMD:
            if (msg_in.uuid != client_uuid) { //Invalid client UUID
                sprintf(msg_out.message, "[ERROR] Invalid client UUID. Connection rejected");
                break;
            }
            sprintf(msg_out.message, "[ERROR] Invalid command");
            break;

        /* CONNECT */
        case CONNECT:
            if (client_uuid != -1) { //Invalid client UUID
                sprintf(msg_out.message, "[ERROR] Client already connected. Connection rejected");
                break;
            }
            client_uuid = msg_in.uuid;
            sprintf(msg_out.message, "CONNECT");
            printf("Connected to %s:%d [%d]\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port), msg_in.uuid);
            break;

        /* DISCONNECT */
        case DISCONNECT:
            if (client_uuid != msg_in.uuid) { //Invalid client UUID
                sprintf(msg_out.message, "[ERROR] Invalid client UUID. Connection rejected");
                break;
            }
            client_uuid = -1;
            sprintf(msg_out.message, "DISCONNECT");
            printf("Disconnected from %s:%d [%d]\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port), msg_in.uuid);
            break;

        /* GETSECTOR */
        case GETSECTOR:
            if (client_uuid != msg_in.uuid) { //Invalid client UUID
                sprintf(msg_out.message, "[ERROR] Invalid client UUID. Connection rejected");
                break;
            }
            char *ptr = msg_in.message;
            ptr += strlen("GETSECTOR");
            while (*ptr == ' ')
                ptr++;

            int sector = atoi(ptr);
            fseek(floppy, 512 * sector, SEEK_SET);
            fread(&msg_out.message, 512, 1, floppy);
            break;
        }

        /*
         * Send response to the client
         * We need to use sendto() because
         * we are using a UDP socket.
         */
        n = sendto(socket_fd, &msg_out, sizeof(msg_out), 0, (struct sockaddr *)&client_addr, (socklen_t)client_len);
        if (n < 0) { //sendto() failed
            perror("socket:sendto");
            continue;
        }
        printf("%s:%d <<< %s\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port), (unsigned char *)msg_out.message);
        fflush(stdout); //Safety precaution
    }
}
