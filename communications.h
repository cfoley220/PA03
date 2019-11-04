#define BUFFER_MAX_SIZE 4096
#define OLD_USER_STATUS 1
#define NEW_USER_STATUS 0
#define BAD_PASSWORD_STATUS 0
#define GOOD_PASSWORD_STATUS 1

enum Operation {BROADCAST, PRIVATE, HISTORY, EXIT};

void debug(char *message) {
  printf("THREAD %d: %s", pthread_self(), message);

  fflush(stdout);
}

int send_buffer(int clientSocket, char *buffer, int size) {
  int len;
  if ((len = write(clientSocket, buffer, size)) == -1) {
    perror("Client Send\n");
    exit(1);
  }
  return len;
}

int receive_buffer(int clientSocket, char * buffer, int size) {
  int len;
  bzero(buffer, sizeof(buffer));
  if ((len = read(clientSocket, buffer, size)) == -1) {
    perror("Client Receive!");
    exit(1);
  }
  return len;
}

int send_int(int clientSocket, int value) {
  int len;
  uint32_t temp = htonl(value);
  if ((len = write(clientSocket, &temp, sizeof(uint32_t))) == -1) {
    perror("Client Send");
    exit(1);
  }

  return len;
}

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

// PROTOCOL FOR SENDING AND RECIEVING STRINGS:
  // Sending:
      // Send the number of characters (NOT INCLUDING NULL CHARACTER)
      // Send those characters
  // Receive
      // Listen for the size
      // Declare a buffer[size+1] (for room for null)
      // Bzero
      // recieve_buffer(socket, buffer, SIZE)
      // buffer[size] = '\0'

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

char * receive_string(int clientSocket) {
  // Listen for the size
  int size = receive_int(clientSocket);

  // Allocate buffer
  char * buffer = malloc(sizeof(char)*(size + 1));

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
