/*
* chatclient.c
* PA03 project
* CSE 30264 - Computer Networks - Fall 2019
* bblum1, cfoley, cmarkley
*
* This is the client side for our chat application. After connecting to the
* server, it first handles logging into the chat server. It then creates a
* listening thread. This listening thread reads from the TCP socket connected
* to the server, and handles incoming messages. The main thread continues on to
* handling user interaction, prompting the user for operations and handling them.
* The client handles broadcast messaging, private messaging, chat history, and
* exiting.
*/

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
#include "../parameters.h"
#include "../communications.h"

// Flags used to allow thread communication
bool acknowledged, confirmation, success;

pthread_mutex_t promptMutex = PTHREAD_MUTEX_INITIALIZER;


// Variable the holds that last prompt sent to the user.
char lastPrompt[BUFFER_MAX_SIZE];

/*
* Send a prompt to the user. Automatically saves the prompt as the last prompt used
*/
void prompt(char * p){
  fflush(stdout);
  printf("%s", p);

  pthread_mutex_lock(&promptMutex);
  strcpy(lastPrompt, p);
  pthread_mutex_unlock(&promptMutex);

}

/*
* Handler for the listening thread to recieve messages from the server
*/
void* receive_messages(void* socket) {
  // debug("i am child (listening thread)\n");

  // Convert socket to a usable variable
  int clientSocket = *(int *)socket;

  int count = 0;
  while(true && count++ < 30) { //TODO: remove counter

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

    // printf("Received message:\n~~~\n%s\n~~~\n", message);
    // Handle message
    if (strcmp(message, "ACK") == 0) {

      // Notify main thread of acknowledgment
      acknowledged = true;

    } else if (strcmp(message, "CONF_SUCCESS") == 0){

      // Notify main thread of a successful confirmation
      confirmation = true;
      success = true;

    } else if (strcmp(message, "CONF_FAIL") == 0){

      // Notify main thread of a unsuccessful confirmation
      confirmation = true;
      success = false;

    } else if (strcmp(message, "EXIT") == 0){

      return;

    } else {
      // Message is a standard communcation message

      printf("\n%s\n", message); // TODO: uncomment

      // Display propmt to user
      pthread_mutex_lock(&promptMutex); //TODO: move this above the print?

      // Flush out all of previous typed things
      // TODO: this is not working
      // while (getche() != '\n') { ; }

      // Display new prompt
      printf(lastPrompt);
      // Flush to ensure entire prompt is printed.
      fflush(stdout);
      pthread_mutex_unlock(&promptMutex);

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

  // Return proper Operation based upon input
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
  // Wait for acknowledgment
  while(!acknowledged){
    ;
  }
  acknowledged = false;
  // TODO: replace with thread safe application?
}

/*
* Spin-waits until confirmation flag is set to true.
* Returns the value of the confirmation
*/
bool waitForConfirmation() {
  // Wait for acknowledgment
  while(!confirmation){
    ;
  }
  confirmation = false;
  return success;
  // TODO: replace with thread safe application?
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

  // Prepare max buffer
  char maxBuffer[BUFFER_MAX_SIZE];
  bzero(maxBuffer, sizeof(maxBuffer));

  /*************************
  *  Log in or create user *
  **************************/

  // Send username to server
  send_string(clientSocket, username);
  printf("Sent username: (%s)\n", username);

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
  send_string(clientSocket, maxBuffer);
  printf("Sent password (%s)\n", maxBuffer);

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

  /***************************
  *  Create listening thread *
  ****************************/

  pthread_t listeningThread;
  pthread_create(&listeningThread, NULL, receive_messages, (void*) &clientSocket);

  // debug("i am main thread\n");

  /**********************
  *  Interact with user *
  **********************/

  while(1) {

    // Get operation from user
    enum Operation operation = getOperation();

    // Send operation to server
    send_int(clientSocket, operation);

    switch (operation) {

      case BROADCAST:

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


        if(!waitForConfirmation()){
          printf("No other users are currently online.\n");
          break;
        };

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

        bzero(lastPrompt, sizeof(lastPrompt));

        waitForAcknowledgement();

        break;

      case EXIT:

        printf("exiting\n");
        // TODO: how to exit the while loop
        printf("waiting for listeningThread to end\n");
        pthread_join(listeningThread, NULL);
        close(clientSocket);
        return;
    }
  }

  debug("waiting for thread to end");


}
