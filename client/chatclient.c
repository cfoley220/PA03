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

void* receive_messages(void* socket) {
  debug("i am child (listening thread)");

  int clientSocket = *(int *)socket;

  debug("socket is: --\n");
  printf("\t%d\n", clientSocket);

  // Receive size of message to receive
  int size = receive_int(clientSocket);

  if (size < 0) {
    debug("something went wrong"); //TODO
  }

  // Receive message
  char message[size+1];
  bzero(message, sizeof(message));
  receive_buffer(clientSocket, message, size);
  message[size] = '\0';

  // Print message
  printf("%s", message);


  // Print prompt
  // TODO
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


int main(int argc, char *argv[]) {

  // Check for proper usage
  if (argc != 4) {
    printf("%s: Incorrect usage.\n Usage: %s ADDR PORT USERNAME \n", argv[0], argv[0]);
    exit(1);
  }



  /*********************
  *  Connect to server *
  **********************/

  // Create socket
	int clientSocket;

	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0 )  {
		printf("Client: Error creating socket\n");
		exit(0);
	}

  // Assign proper commandline variables
  char* hostname = argv[1];
  int port       = atoi(argv[2]);
  char* username = argv[3];

  // Set up server address
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_port         = htons(port);
  servaddr.sin_family       = AF_INET;
  servaddr.sin_addr.s_addr  = inet_addr(inet_ntoa(*((struct in_addr*) gethostbyname(argv[1])->h_addr_list[0])));

  // Connect
  if (connect(clientSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    printf("Client: Error connecting to server: %s\n", strerror(errno));
    exit(0);
  }

  debug("Connection established\n");
  // Prepare max buffer
  char maxBuffer[BUFFER_MAX_SIZE];
  bzero(maxBuffer, sizeof(maxBuffer));


  /*************************
  *  Log in or create user *
  **************************/

  // Send username to server
  printf("Sending username. (%s)\n", username);
  send_string(clientSocket, username);
  printf("Sent username.\n");

  // Receive response from server
  int userAccountStatus = receive_int(clientSocket);

  // Handle status
  if (userAccountStatus == OLD_USER_STATUS) {
    printf("\tgot status of OLD_USER_STATUS\n");
  } else if (userAccountStatus == NEW_USER_STATUS){
    printf("\tgot status of NEW_USER_STATUS\n");
  } else {
    printf("you should never see this. user status fucked\n");
  }

  // Request password
  bzero(maxBuffer, sizeof(maxBuffer));
  printf("Please enter your password: "); // TODO: make this match the demo video

  if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
    debug("Failed to get user input");
  }

  printf("Sending password (%s)\n", maxBuffer);

  send_string(clientSocket, maxBuffer);
  printf("Sent password\n");

  // Handle status
  int passwordStatus = receive_int(clientSocket);

  if (passwordStatus == BAD_PASSWORD_STATUS) {
    printf("\t got status of BAD_PASSWORD_STATUS\n");
  } else if (passwordStatus == GOOD_PASSWORD_STATUS) {
    printf("\t got status of GOOD_PASSWORD_STATUS\n");
  } else {
    printf("you should never see this print statement. pass status fucked up.\n");
  }

  printf("----------------------\n");
  printf("Handling statuses now\n");

  if (userAccountStatus == OLD_USER_STATUS) {
    printf("userAccountStatus is true\n");
    while (passwordStatus == BAD_PASSWORD_STATUS) {
      printf("passowrd status is false\n");
      // Request password
      bzero(maxBuffer, sizeof(maxBuffer));
      printf("Password incorrect. Please enter your password: "); // TODO: make this match the demo video

      if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
        debug("Failed to get user input\n");
      }

      printf("got password: %s\n", maxBuffer);

      send_string(clientSocket, maxBuffer);

      // Handle status
      int passwordStatus = receive_int(clientSocket);
    }
    printf("while loop done\n");

  } else if (passwordStatus == BAD_PASSWORD_STATUS) {
    debug("Failed to register.\n");

    printf("userAccountStatus is false and passwordStatus is false\n");

    exit(1);
    // TODO: ensure this is the behavior we want
  }

  debug("done with username and password shit.\n");

  /***************************
  *  Create listening thread *
  ****************************/
  debug("creating new thread\n");

  pthread_t clientThread;
  pthread_create(&clientThread, NULL, receive_messages, (void*) &clientSocket);

  debug("i am main thread\n");

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


  pthread_join(clientThread, NULL);

}
