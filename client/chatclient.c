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

// Flags used to allow thread communication
bool acknowledged, confirmation, success, promptReady;

// Variable the holds that last prompt sent to the user.
char lastPrompt[BUFFER_MAX_SIZE];

/*
* Send a prompt to the user. Automatically saves the prompt as the last prompt used
*/
void prompt(char * p){
  printf("%s", p);

  promptReady = false;
  strcpy(lastPrompt, p);
  promptReady = true;

}

/*
* Handler for the listening thread to recieve messages from the server
*/
void* receive_messages(void* socket) {
  debug("i am child (listening thread)\n");

  // Convert socket to a usable variable
  int clientSocket = *(int *)socket;

  int count = 0;
  while(true && count++ < 30) { //TODO: remove counter

    // Receive size of message to receive
    int size = receive_int(clientSocket);

    debug("I recieved a size from the server.\n");
    if (size < 0) {
      debug("something went wrong"); //TODO
    }

    // Receive message
    char message[size+1];
    bzero(message, sizeof(message));
    receive_buffer(clientSocket, message, size);
    message[size] = '\0';

    debug("I recieved a buffer from the server\n");

    printf("DEBUG: got message: %s\n", message); // TODO: delete

    // Handle message
    if (strcmp(message, "ACK") == 0) {
      acknowledged = true;
    } else if (strcmp(message, "CONF_SUCCESS") == 0){
      confirmation = true;
      success = true;
    } else if (strcmp(message, "CONF_FAIL") == 0){
      confirmation = true;
      success = false;
    } else {
      // Print message

      // printf("%s", message); // TODO: uncomment

      // Wait for prompt to be set
      while (!promptReady) {
        ;
      }
      printf(lastPrompt);
    }

  }
}

/*
* Checks if a string is a valid operation choice for a command
*/
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

/*
* Prompts the user for a operation and converts it to a proper Operation
*/
enum Operation getOperation(){
  prompt("Please type in a operation (B, P, H, X):\n>   ");

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
    prompt("Please type in a valid operation (B, P, H, X):\n>   ");

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

/*
* Spin-waits until acknowledgment flag is set to true. Resets flag to false
*/
void waitForAcknowledgement() {
  debug("Waiting for acknowledgment\n");
  // Wait for acknowledgment
  while(!acknowledged){
    ;
  }
  acknowledged = false;
  // TODO: replace with thread safe application?
  debug("Received acknowledgment\n");
}

/*
* Spin-waits until confirmation flag is set to true.
* Returns the value of the confirmation
*/
bool waitForConfirmation() {
  debug("Waiting for confirmation\n");
  // Wait for acknowledgment
  while(!confirmation){
    ;
  }
  confirmation = false;
  return success;
  // TODO: replace with thread safe application?
  debug("Received confirmation\n");
}

int main(int argc, char *argv[]) {

  // Check for proper usage
  if (argc != 4) {
    printf("%s: Incorrect usage.\n Usage: %s ADDR PORT USERNAME \n", argv[0], argv[0]);
    exit(1);
  }

  // Initialize variables
  acknowledged = false;
  confirmation = false;
  success      = false;
  promptReady  = false;



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
        waitForAcknowledgement();

        // Get message
        bzero(maxBuffer, sizeof(maxBuffer));
        prompt("Please enter your message:\n>   "); // TODO: make this match the demo video

        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        // Remove newline
        maxBuffer[strlen(maxBuffer) - 1] = '\0';

        send_string(clientSocket, maxBuffer);

        // Wait for acknowledgment
        if(waitForConfirmation()){
          printf("Sent successfully\n");
        } else {
          printf("Failed to send\n");
        }

        break;
      case PRIVATE:
        printf("privating\n");

        // At this point, the server sends a list of active users.

        // Get username
        bzero(maxBuffer, sizeof(maxBuffer));
        prompt("Please enter the username you'd like to message:\n>   "); // TODO: make this match the demo video

        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        // Remove newline
        maxBuffer[strlen(maxBuffer) - 1] = '\0';

        send_string(clientSocket, maxBuffer);

        // Get message
        bzero(maxBuffer, sizeof(maxBuffer));
        prompt("Please enter your message:\n>   "); // TODO: make this match the demo video

        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        // Remove newline
        maxBuffer[strlen(maxBuffer) - 1] = '\0';

        send_string(clientSocket, maxBuffer);

        if (waitForConfirmation()) {
          printf("Message sent successfully\n"); // TODO: make this match the demo video
        } else {
          printf("User did not exist\n"); // TODO: make this match the demo video
        }

        break;
      case HISTORY:
        printf("Historying\n");
        break;
      case EXIT:
        printf("exiting\n");
        close(clientSocket);
        // TODO: how to exit the while loop
        printf("waiting for listeningThread to end\n");
        pthread_join(listeningThread, NULL);
        return;
    }
  }

  debug("waiting for thread to end");


}
