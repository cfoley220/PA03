// Header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include<pthread.h>
#include <unistd.h>
#include "pg3lib.h"

// Define variables
#define BUFFER_MAX_SIZE 4096

/*******************
*  Set up client   *
*******************/

// Connect to server
// Prompt for and send username
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

  // Create socket
	int socket;
	if((socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )  {
		printf("Client: Error creating socket\n");
		exit(0);
	}

  char * hostname = "student02.cse.nd.edu";
  int port = 41030;

  // Set up server address
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*) gethostbyname(hostname)->h_addr_list[0])));
  servaddr.sin_port = htons(port);

  // connect
  if (connect(socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    printf("Client: Error connecting to server: %s\n", strerror(errno));
    exit(0);
  }
  printf("Connection established\n");

}
