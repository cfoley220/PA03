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

//Sending and Receiving Functions
int send_buffer(int clientSocket, char *buffer, int size) {
  int len;
  if ((len = write(clientSocket, buffer, size)) == -1) {
    perror("Client Send\n");
    exit(1);
  }
  return len;
}

int receive_buffer(int clientSocket, char *buffer, int size) {
  int len;
  bzero(buffer, sizeof(buffer));
  if ((len = read(clientSocket, buffer, size)) == -1) {
    perror("Client Receive!");
    exit(1);
  }
  return len;
}

int send_int(int value, int clientSocket) {
  int len;
  uint32_t temp = htonl(value);
  if ((len = write(clientSocket, &temp, sizeof(temp))) == -1) {
    perror("Client Send");
    exit(1);
  }

  return len;
}

int receive_int(int clientSocket) {
  int buffer;
  int len;
  bzero(&buffer, sizeof(buffer));
  if ((len = read(clientSocket, &buffer, sizeof(buffer))) == -1) {
    perror("Client Receive Error");
    exit(1);
  }

  int temp = ntohl(buffer);
  return temp;
}

void* receive_messages(void* socket) {
  
}

/*******************
*  Set up client   *
*******************/

// Connect to server
// Prompt for and send username
//Based on the video, we need to first send username and then server
//will check to see if the user is new or returning, and prompt the
//client accordingly.
// Prompt for and send password
// Handle response
  // If fail, prompt for and send password again

// Create thread to listen for messages

/**********************
*  Interact with user *
**********************/
// Loop until client exits

  // Update most recent prompt to main prompt
  // Display main prompt

  // Accept command

  // Switch
  // B:
    //
  // P:
    //
  // H:
    // Request history from server
  // X:
    // quit


int main(int argc, char *argv[]) {

  //Check for proper usage
  if (argc != 4) {
    printf("%s: Incorrect usage.\n Usage: %s ADDR PORT USERNAME \n", argv[0], argv[0]);
    exit(1);
  }

  // Create socket
	int clientSocket;
	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )  {
		printf("Client: Error creating socket\n");
		exit(0);
	}

  //Assign proper commandline variables
  char* hostname = argv[1];
  int port = atoi(argv[2]);
  char* username = argv[3];
  char buffer[BUFFER_MAX_SIZE];
  //char* hostname = "student02.cse.nd.edu";
  //int port = 41030;

  // Set up server address
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*) gethostbyname(hostname)->h_addr_list[0])));
  servaddr.sin_port = htons(port);

  // connect
  if (connect(clientSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    printf("Client: Error connecting to server: %s\n", strerror(errno));
    exit(0);
  }
  printf("Connection established\n");


  //Create listening thread
  pthread_t clientThread;
  pthread_create(&clientThread, NULL, receive_messages, (void*) &clientSocket);

  //Send username to server
  bzero(buffer, sizeof(buffer));
  sprintf(buffer, username);
  buffer[BUFFER_MAX_SIZE] = '\0';
  sendbuf(buffer, clientSocket, strlen(buffer));



}
