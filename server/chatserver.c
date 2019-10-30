// Header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

// Define variables
#define BUFFER_MAX_SIZE 4096

void* connection_handler(void*);

//Sending and Receiving Functions
int send_buffer(int clientSocket, char *buffer, int size) {
    int len;
    if ((len = write(clientSocket, buffer, size)) == -1) {
        perror("Server Send\n");
        exit(1);
    }
    return len;
}

int receive_buffer(int clientSocket, char *buffer, int size) {
    int len;
    bzero(buffer, sizeof(buffer));
    if ((len = read(clientSocket, buffer, size)) == -1) {
        perror("Server Receive!");
        exit(1);
    }
    return len;
}

int send_int(int value, int clientSocket) {
    int len;
    uint32_t temp = htonl(value);
    if ((len = write(clientSocket, &temp, sizeof(temp))) == -1) {
        perror("Server Send");
        exit(1);
    }

    return len;
}

int receive_int(int clientSocket) {
    int buffer;
    int len;
    bzero(&buffer, sizeof(buffer));
    if ((len = read(clientSocket, &buffer, sizeof(buffer))) == -1) {
        perror("Server Receive Error");
        exit(1);
    }

    int temp = ntohl(buffer);
    return temp;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("%s: Incorrect usage.\n Usage: %s PORT \n", argv[0], argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    /* Initialize variables */
    int sockfd, clientAddrLen;
    struct sockaddr_in serverAddr, clientAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    /* Establish the server's own ip address */
    char hostBuffer[256];

    if (gethostname(hostBuffer, sizeof(hostBuffer)) < 0) {
        fprintf(stderr, "%s: Failed to get current host name\n", argv[0]);
        exit(-1);
    };

    char *IPbuffer = inet_ntoa(*((struct in_addr *)gethostbyname(hostBuffer)->h_addr_list[0]));

    serverAddr.sin_addr.s_addr = inet_addr(IPbuffer);

    /* Initialize server to accept incoming connections */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s: Failed to call socket\n", argv[0]);
        exit(-1);
    }

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "%s: Failed to bind socket: %s\n", argv[0], strerror(errno));
        exit(-1);
    }

    if ((listen(sockfd, SOMAXCONN)) < 0) {
        fprintf(stderr, "%s: Failed to listen: %s\n", argv[0], strerror(errno));
        exit(-1);
    }

    printf("Waiting for connection on port %d\n", port);

    //int clientSocket;
    int newClient;

    //create users and active clients files (signed in vs all)
    pthread_t serverThread;
    FILE* fp;
    fp = fopen("Clients.txt", "wr");
    fclose(fp);
    fp = fopen("Users.txt", "wr");
    fclose(fp);

    //creates a thread for every incoming connection from a client
    while ((newClient = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen))) {
        printf("Connection established\n");
        if (pthread_create(&serverThread, NULL, connection_handler, (void*) &newClient) < 0) {
            perror("ERROR: Couldn't create thread!");
            exit(1);
        }
    }

    if (newClient < 0) {
        perror("Accept failed\n");
        exit(1);
    }
    return 0;

}

void* connection_handler(void* socket) {
    int new_s = *(int*)socket;
    char buffer[BUFFER_MAX_SIZE];
    int rec;

    //Receive username
    char username[BUFFER_MAX_SIZE];
    bzero((char *)&username, sizeof(username));
    recvbuf(new_s, username, sizeof(username));
    username[strlen(username) - 1] = '\0';
    printf("%s", username);
}