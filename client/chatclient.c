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
#include <stdbool.h>
#include "../communications.h"

void* receive_messages(void* socket) {
  debug("i am child (listening thread)");

  int clientSocket = *(int *)socket;

  debug("socket is: --\n");
  printf("\t%d\n", clientSocket);

  // Receive size of message to receive
  int size = receive_int(clientSocket);

  debug("got a size");
  if (size < 0) {
    debug("something went wrong"); //TODO
  }

  // Receive message
  char message[size+1];
  bzero(message, sizeof(message));
  receive_buffer(clientSocket, message, size);
  message[size] = '\0';
  printf("recieved something within listening thread\n");

  // Print message
  printf("%s", message);


  // Print prompt
  // TODO
}

bool isValidOperation(char * op){

  if (strcmp(op, "B") != 0 &&
      strcmp(op, "P") != 0 &&
      strcmp(op, "H") != 0 &&
      strcmp(op, "X") != 0
    ) {
    return false;
  }

  return true;
}

enum Operation getOperation(){
  printf("Please type in a operation (B, P, H, X):   \n");

  // Prepare max buffer
  char maxBuffer[BUFFER_MAX_SIZE];
  bzero(maxBuffer, sizeof(maxBuffer));

  if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
    debug("Failed to get user input");
  }

  // Remove new line
  maxBuffer[strlen(maxBuffer) - 1] = '\0';

  // Continue to prompt for operation until a valid operation is recieved
  while (!isValidOperation(maxBuffer)) {
    printf("Please type in a valid operation (B, P, H, X):   \n");

    bzero(maxBuffer, sizeof(maxBuffer));

    if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
      debug("Failed to get user input");
    }

    // Remove new line
    maxBuffer[strlen(maxBuffer) - 1] = '\0';
  }

  if (strcmp(maxBuffer, "B") == 0) {
    return BROADCAST;
  } else if (strcmp(maxBuffer, "P") == 0) {
    return PRIVATE;
  } else if (strcmp(maxBuffer, "H") == 0) {
    return HISTORY;
  } else if (strcmp(maxBuffer, "X") == 0) {
    return EXIT;
  } else {
    debug("Received invalid operation somehow.");
    exit(1);
  }

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

  // Request password
  bzero(maxBuffer, sizeof(maxBuffer));
  printf("Please enter your password: "); // TODO: make this match the demo video

  if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
    debug("Failed to get user input");
  }

  // Remove new line
  maxBuffer[strlen(maxBuffer) - 1] = '\0';


  // Send password
  printf("Sending password (%s)\n", maxBuffer);
  send_string(clientSocket, maxBuffer);

  // Handle status
  int passwordStatus = receive_int(clientSocket);

  if (passwordStatus == BAD_PASSWORD_STATUS) {
    printf("Failure to login\n");
    exit(1);
  } else if (userAccountStatus == NEW_USER_STATUS) {
    printf("User creation successful\n");
  } else {
    printf("Successful login. Welcome back.\n");
  }

  debug("done with username and password shit.\n");

  /***************************
  *  Create listening thread *
  ****************************/

  debug("creating new thread\n");

  pthread_t listeningThread;
  pthread_create(&listeningThread, NULL, receive_messages, (void*) &clientSocket);

  debug("i am main thread\n");

  /**********************
  *  Interact with user *
  **********************/
  int counter = 0; //TODO: delete counter
  while(1 && counter++ < 10) {

    // Get operation from user
    enum Operation operation = getOperation();

    // Send operation to server
    send_int(clientSocket, operation);

    switch (operation) {
      case BROADCAST:
        printf("Broadcasting\n");
        
        // Wait for acknowledgment
        receive_int(clientSocket);

        // Get message
        bzero(maxBuffer, sizeof(maxBuffer));
        printf("Please enter your message: "); // TODO: make this match the demo video

        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        send_string(clientSocket, maxBuffer);

        // Wait for confirmation
        int status = receive_int(clientSocket);

        if (status) {
          printf("Successfully sent\n");
        } else {
          printf("something didnt work ): sorry\n");
        }

        break;
      case PRIVATE:
        printf("privating\n");
        break;
      case HISTORY:
        printf("Historying\n");
        break;
      case EXIT:
        printf("exiting\n");
        break;
    }
  }
  // enum opertaion = getOperation();

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


  pthread_join(listeningThread, NULL);

}
