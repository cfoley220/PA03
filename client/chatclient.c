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

// Lock to block printing the prompt
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
  // Convert socket to a usable variable
  int clientSocket = *(int *)socket;

  while(true) {

    // Receive size of message to receive
    // int size = receive_int(clientSocket);
    //
    // if (size < 0) {
    //   printf("Recieved a negative size from server.\n");
    //   exit(1);
    // }
    //
    // // Receive message
    // char message[size+1];
    // bzero(message, sizeof(message));
    // receive_buffer(clientSocket, message, size);
    // message[size] = '\0';

    char * message = receive_string(clientSocket);

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
      free(message);
      return;

    } else {
      /* Message is a standard communcation message */

      // Lock the terminal prompt
      pthread_mutex_lock(&promptMutex);

      // Print message
      printf("\n\n%s\n\n", message);

      // Display prompt again
      printf(lastPrompt);

      // Flush to ensure entire prompt is printed.
      fflush(stdout);

      // Unlock the terminal prompt
      pthread_mutex_unlock(&promptMutex);

    }
    free(message);
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
  prompt("Enter P for private conversation.\nEnter B for message broadcasting.\nEnter H for chat history.\nEnter X for Exit\n>> ");

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

  // Receive response from server
  int userAccountStatus = receive_int(clientSocket);

  // Request password
  bzero(maxBuffer, sizeof(maxBuffer));

  if (userAccountStatus == NEW_USER_STATUS) {
    printf("New user? Create password >>  ");
  } else {
    printf("Welcome Back! Enter password >>  ");
  }

  if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
    debug("Failed to get user input");
  }

  // Remove new line
  maxBuffer[strlen(maxBuffer) - 1] = '\0';


  // Send password
  send_string(clientSocket, maxBuffer);

  int passwordStatus = receive_int(clientSocket);

  // Handle status
  if (userAccountStatus == OLD_USER_STATUS) {
    if (passwordStatus == GOOD_PASSWORD_STATUS) {
      /* User is returning user with correct password */
      printf("Welcome %s!\n", username);

    } else {

      /* User is returning user with incorrct password */
      while (passwordStatus == BAD_PASSWORD_STATUS) {

        // Request password
        printf("Invalid password.\n Please enter again >> ");

        bzero(maxBuffer, sizeof(maxBuffer));
        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        // Remove new line
        maxBuffer[strlen(maxBuffer) - 1] = '\0';

        // Send password
        send_string(clientSocket, maxBuffer);

        passwordStatus = receive_int(clientSocket);
      }
      printf("Welcome %s!\n", username);
    }

  } else {
    if (passwordStatus == GOOD_PASSWORD_STATUS) {
      /* New user with a good password */
        printf("Welcome %s! Registration Complete.\n", username);
    } else {
      /* New user with a bad password (shouldnt occur) */
      debug("Something went wrong.\n");
      exit(1);
    }
  }

  /***************************
  *  Create listening thread *
  ****************************/

  pthread_t listeningThread;
  pthread_create(&listeningThread, NULL, receive_messages, (void*) &clientSocket);

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
        prompt("Enter Broadcast Message >>  ");

        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        // Remove newline
        maxBuffer[strlen(maxBuffer) - 1] = '\0';

        send_string(clientSocket, maxBuffer);

        // Wait for acknowledgment
        if(waitForConfirmation()){
          printf("Messaged Broadcasted.\n");
        } else {
          printf("Failed to send.\n");
        }

        break;

      case PRIVATE:

        // Cancels out the last propmt stop premptive printing of wrong prompt
        bzero(lastPrompt, sizeof(lastPrompt));

        if(!waitForConfirmation()){
          printf("No other users are currently online.\n");
          break;
        };



        // At this point, the server sends a list of active users.

        // Client waits for acknowledgement to ensure the list arrived before
        // displaying the next prompt.
        waitForAcknowledgement();

        // Get username
        bzero(maxBuffer, sizeof(maxBuffer));
        prompt("Enter User Name >>   ");

        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        // Remove newline
        maxBuffer[strlen(maxBuffer) - 1] = '\0';

        // send username to send message to
        send_string(clientSocket, maxBuffer);

        // Ensure username works
        while(!waitForConfirmation()) {
          prompt("User does not exist. Please re-enter username:  ");
          bzero(maxBuffer, sizeof(maxBuffer));

          if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
            debug("Failed to get user input");
          }

          // Remove newline
          maxBuffer[strlen(maxBuffer) - 1] = '\0';

          // send username to send message to
          send_string(clientSocket, maxBuffer);
        }

        // Get message
        bzero(maxBuffer, sizeof(maxBuffer));
        prompt("Enter Private Message >>   ");

        if ((fgets(maxBuffer, sizeof(maxBuffer), stdin)) < 0) {
          debug("Failed to get user input");
        }

        // Remove newline
        maxBuffer[strlen(maxBuffer) - 1] = '\0';

        // Send message to be sent
        send_string(clientSocket, maxBuffer);

        if (waitForConfirmation()) {
          printf("Message Sent.\n");
        } else {
          printf("Something went wrong.\n");
        }

        break;

      case HISTORY:

        bzero(lastPrompt, sizeof(lastPrompt));

        waitForAcknowledgement();

        break;

      case EXIT:
        //printf("Goodbye!\n");
        pthread_join(listeningThread, NULL);
        close(clientSocket);
        return;
    }
  }
}
