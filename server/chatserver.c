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
#include <arpa/inet.h>
#include "../communications.h"

void* connection_handler(void*);

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

    char* IPbuffer = inet_ntoa(*((struct in_addr*) gethostbyname(hostBuffer)->h_addr_list[0]));



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
    //FILE* fp;
    // fp = fopen("Clients.txt", "wr");
    // fclose(fp);
    // fp = fopen("Users.txt", "wr");
    // fclose(fp);

    //creates a thread for every incoming connection from a client
    while ((newClient = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen))) {
        printf("Connection established\n");
        if (pthread_create(&serverThread, NULL, connection_handler, (void*) &newClient) < 0) {
            perror("ERROR: Couldn't create thread!\n");
            exit(1);
        }

    }

    if (newClient < 0) {
        perror("Accept failed\n");
        exit(1);
    }

    return 0;

}

int search(FILE* fp, char* username) {
  //fseek(fp, 0, SEEK_SET);
  char temp[BUFFER_MAX_SIZE];
  bzero((char *)&temp, sizeof(temp));
  while(fgets(temp, sizeof(temp), fp)) {
    if (strcmp(temp, "") != 0) {
      char* temp2;
      bzero((char *)&temp2, sizeof(temp2));
      temp2 = strtok(temp, ":");
      if (strcmp(temp2, username) == 0) {
        return OLD_USER_STATUS;
      }
    }
    bzero((char *)&temp, sizeof(temp));
  }
  return NEW_USER_STATUS;
}

int check_password(FILE* fp, char* username, char* password) {
  //fseek(fp, 0, SEEK_SET);
  char temp[BUFFER_MAX_SIZE];
  bzero((char *)&temp, sizeof(temp));
  while(fgets(temp, sizeof(temp), fp)) {
    printf("The current line in users file: %s\n", temp);
    char* temp2;
    bzero((char *)&temp2, sizeof(temp2));
    temp2 = strtok(temp, ":");
    printf("username in file: %s\n", temp2);
    if (strcmp(temp2, username) == 0) {
      temp2 = strtok(NULL, ":");
      //temp2[strlen(temp2) - 1] = '\0';
      printf("password in file: %s\n", temp2);
      if (strcmp(temp2, password) == 0) {
        return GOOD_PASSWORD_STATUS;
      }
    }
    bzero((char *)&temp, sizeof(temp));
  }
  return BAD_PASSWORD_STATUS;
}

int new_user(FILE *fp, char* username, char* password){
  //fseek(fp, 0, SEEK_END);
  debug("in new user function\n");
  char line[BUFFER_MAX_SIZE];
  sprintf(line, "%s:%s", username, password);
  printf("line to be added to the file: %s\n", line);
  int status = fputs(line, fp);
  printf("status of adding it to the file %d\n", status);

  if(status < 0) {
    return BAD_PASSWORD_STATUS;
  }

  return GOOD_PASSWORD_STATUS;
}

void* connection_handler(void* socket) {
    int clientSocket = *(int*)socket;
    int rec;
    debug("About to try to recieve string\n");

    // Receive username
  
    char *username = receive_string(clientSocket);
    //username[strlen(username) - 1] = '\0';
    printf("username: %s\n", username);

    // Check if the user exists already

    FILE *fp = fopen("Users.txt", "r");
    int found = search(fp, username);
    printf("Status of found: %d", found);
    fclose(fp);
  
    send_int(clientSocket, found);

    if (found) { // user exists
      // receive password
      char *password = receive_string(clientSocket);
      //password[strlen(password) - 1] = '\0';

      // check if password if correct
      //fp =fopen("Users.txt", "r");
      int correct = check_password(fp, username, password);
      //fclose(fp);

      // send status of if password was correct
      send_int(clientSocket, correct);

    } else { // user DNE
      // receive new password
      char *password = receive_string(clientSocket);
      //password[strlen(password) - 1] = '\0';

      printf("Password received: %s\n", password);

      // write new user/password to the file
      fp = fopen("Users.txt", "a");
      int status = new_user(fp, username, password);
      fclose(fp);

      debug("Wrote new user to the file\n");

      // send status of creating new user
      send_int(clientSocket, status);

      debug("Sent status\n");

    }

    //send_int(clientSocket, 5);
    //send_buffer(clientSocket, "hello", 5);


    // make sure this happens after we finish using username
    free(username);


}
