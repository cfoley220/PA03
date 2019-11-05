/*
* parameters.h
* PA03 project
* CSE 30264 - Computer Networks - Fall 2019
* bblum1, cfoley, cmarkley
*
* This file holds parameters used for this project. Also holds debugging tools
*/

// Default max buffer size
#define BUFFER_MAX_SIZE 4096

// Status to represent if users existed already or not
#define OLD_USER_STATUS 1
#define NEW_USER_STATUS 0

// Status to represent password correctness
#define BAD_PASSWORD_STATUS 0
#define GOOD_PASSWORD_STATUS 1

// Enum to represent the operations handled by our client and server
enum Operation {BROADCAST, PRIVATE, HISTORY, EXIT};

/*
* Function used for debug. prints out the string along with the thread id.
*/
void debug(char *message) {
  printf("THREAD %d: %s", pthread_self(), message);

  fflush(stdout);
}
