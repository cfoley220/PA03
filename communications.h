#define BUFFER_MAX_SIZE 4096
#define OLD_USER_STATUS 1
#define NEW_USER_STATUS 0
#define BAD_PASSWORD_STATUS 0
#define GOOD_PASSWORD_STATUS 1

enum Operation {BROADCAST, PRIVATE, HISTORY, EXIT};

/*
* Function used for debug. prints out the string along with the thread id.
*/
void debug(char *message) {
  printf("THREAD %d: %s", pthread_self(), message);

  fflush(stdout);
}

/*
* Sends `size` amount of `buffer` via TCP connection to `clientSocket`
*/
int send_buffer(int clientSocket, char *buffer, int size) {
  int len;
  if ((len = write(clientSocket, buffer, size)) == -1) {
    perror("Client Send\n");
    exit(1);
  }
  return len;
}

/*
* Listens on the TCP connection on `clientSocket`.
* Reads in `size` amount of data into `buffer`
*/
int receive_buffer(int clientSocket, char * buffer, int size) {
  int len;
  bzero(buffer, sizeof(buffer));
  if ((len = read(clientSocket, buffer, size)) == -1) {
    perror("Client Receive!");
    exit(1);
  }
  return len;
}

/*
* sends an int to clientSocket via TCP connection
*/
int send_int(int clientSocket, int value) {
  int len;
  uint32_t temp = htonl(value);
  if ((len = write(clientSocket, &temp, sizeof(uint32_t))) == -1) {
    perror("Client Send");
    exit(1);
  }

  return len;
}

/*
* Listens for an int on a TCP connection with clientSocket
*/
int receive_int(int clientSocket) {
  int buffer;
  int len;
  bzero(&buffer, sizeof(buffer));
  if ((len = read(clientSocket, &buffer, sizeof(uint32_t))) == -1) {
    perror("Client Receive Error");
    exit(1);
  }

  int temp = ntohl(buffer);
  return temp;
}

/*
* Abstraction of send_buffer and send_int to allow a message to send to clientSocket
* with automatic sizing. It sends the number of characters within the message
* (excluding the null chracter). It then sends those exact characters over a
* TCP connection to `clientSocket`.
* Complement of receive_string().
*/
void send_string(int clientSocket, char * message) {
  // Send the number of characters (NOT INCLUDING NULL CHARACTER)

  if(send_int(clientSocket, strlen(message)) < 0) {
    debug("Failed to send int");
    // TODO: exit here
  };


  // Send those characters
  if(send_buffer(clientSocket, message, strlen(message)) < 0) {
    debug("Failed to send buffer\n");
    // TODO: exit here
  };
}

/*
* Abstraction of receive_buffer and receive_int to allow receiving of a message
* sent via send_string().
* Listens for the size of the message.
* Declares a buffer of [size+1] to make room for the null
* Receives the characters
* Sets buffer[size] to null character.
*/
char * receive_string(int clientSocket) {
  // Listen for the size
  int size = receive_int(clientSocket);

  // Allocate buffer
  char * buffer = malloc(sizeof(char)*(size + 1)); // TODO enter some fucking frees()

  bzero(buffer, sizeof(buffer));

  // Receive message
  if(receive_buffer(clientSocket, buffer, size) < 0){
    debug("Failed to receive buffer");
    // TODO: exit here
  }

  // Null terminate string
  buffer[size] = '\0';

  return buffer;
}
