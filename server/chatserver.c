/*
* chatserver.c
* PA03 project
* CSE 30264 - Computer Networks - Fall 2019
* bblum1, cfoley, cmarkley
*
* TODO: file explanation
* TODO: create users and clients file if they do not exist
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
#include "../parameters.h"
#include "../communications.h"

// TODO: wrong password for client caused server to quit  ??

void* connection_handler(void*);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("%s: Incorrect usage.\n Usage: %s PORT \n", argv[0], argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    /* Initialize variables */
    int sockfd, clientAddrLen;
    struct sockaddr_in serverAddr, clientAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    /* Establish the server's own ip address */
    char hostBuffer[256];

    if (gethostname(hostBuffer, sizeof(hostBuffer)) < 0) {
        fprintf(stderr, "%s: Failed to get current host name\n", argv[0]);
        exit(-1);
    };

    char* IPbuffer = inet_ntoa(*((struct in_addr*) gethostbyname(hostBuffer)->h_addr_list[0]));



    serverAddr.sin_addr.s_addr = inet_addr(IPbuffer);

    /* Initialize server to accept incoming connections */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "%s: Failed to call socket\n", argv[0]);
        exit(-1);
    }

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        fprintf(stderr, "%s: Failed to bind socket: %s\n", argv[0], strerror(errno));
        exit(-1);
    }

    if ((listen(sockfd, SOMAXCONN)) < 0) {
        fprintf(stderr, "%s: Failed to listen: %s\n", argv[0], strerror(errno));
        exit(-1);
    }

    printf("Waiting for connection on port %d\n", port);

    //int clientSocket;
    int newClient;

    // Create files
    pthread_t serverThread;

    // Create new clients file. The "w" erases the file if it already existed
    // FILE *fp = fopen("Clients.txt", "w");
    fclose(fopen("Clients.txt", "w"));
    // Open/create users file. The "a" ensures no loss of data if file exists
    // fp = fopen("Users.txt", "a");
    fclose(fopen("Users.txt", "a"));

    //creates a thread for every incoming connection from a client
    while ((newClient = accept(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen))) {
        printf("Connection established\n");
        if (pthread_create(&serverThread, NULL, connection_handler, (void*) &newClient) < 0) {
            perror("ERROR: Couldn't create thread!\n");
            exit(1);
        }
    }

    if (newClient < 0) {
        perror("Accept failed\n");
        exit(1);
    }

    remove("Clients.txt");

    return 0;

}

int search(FILE* fp, char* username) {
  char temp[BUFFER_MAX_SIZE];
  bzero((char *)&temp, sizeof(temp));
  while(fgets(temp, sizeof(temp), fp)) {
    if (strcmp(temp, "") != 0) {
      char* temp2;
      bzero((char *)&temp2, sizeof(temp2));
      temp2 = strtok(temp, ":");
      if (strcmp(temp2, username) == 0) {
        return OLD_USER_STATUS;
      }
    }
    bzero((char *)&temp, sizeof(temp));
  }
  return NEW_USER_STATUS;
}

int check_password(FILE* fp, char* username, char* password) {
  char temp[BUFFER_MAX_SIZE];
  bzero((char *)&temp, sizeof(temp));
  while(fgets(temp, sizeof(temp), fp)) {
    printf("The current line in users file: %s\n", temp);
    char* temp2;
    bzero((char *)&temp2, sizeof(temp2));
    temp2 = strtok(temp, ":");
    printf("username in file: %s\n", temp2);
    if (strcmp(temp2, username) == 0) {
      temp2 = strtok(NULL, ":");
      temp2[strlen(temp2) - 1] = '\0';
      printf("password in file: %s\n", temp2);
      if (strcmp(temp2, password) == 0) {
        printf("Password match\n");
        return GOOD_PASSWORD_STATUS;
      }
    }
    bzero((char *)&temp, sizeof(temp));
  }
  printf("Bad password!");
  return BAD_PASSWORD_STATUS;
}

int new_user(FILE *fp, char* username, char* password){
  debug("in new user function\n");
  char line[BUFFER_MAX_SIZE];
  sprintf(line, "%s:%s\n", username, password);
  printf("line to be added to the file: %s\n", line);
  int status = fputs(line, fp);
  printf("status of adding it to the file %d\n", status);

  // Create a history file for the user
  char filename[BUFFER_MAX_SIZE];
  bzero((char *)&filename, sizeof(filename));
  sprintf(filename, "%s.chat", username);
  FILE* fp_hist = fopen(filename, "w");
  fputs("############## Chat History: ##############\n", fp_hist);
  fclose(fp_hist);

  if(status < 0) {
    return BAD_PASSWORD_STATUS;
  }

  return GOOD_PASSWORD_STATUS;
}

int new_client(FILE* fp, char* username, int my_socket) {
  debug("in new client function\n");
  char line[BUFFER_MAX_SIZE];
  sprintf(line, "%s:%d\n", username, my_socket);
  printf("line to be added to the file: %s\n", line);
  int status = fputs(line, fp);
  printf("status of adding it to the file %d\n", status);

  if(status < 0) {
    return BAD_PASSWORD_STATUS;
  }

  return GOOD_PASSWORD_STATUS;

}

int find_user_socket(FILE* fp, char* username){
  char temp[BUFFER_MAX_SIZE];
  bzero((char *)&temp, sizeof(temp));
  while(fgets(temp, sizeof(temp), fp)) {
    if (strcmp(temp, "") != 0) {
      char* temp2;
      bzero((char *)&temp2, sizeof(temp2));
      temp2 = strtok(temp, ":");
      if (strcmp(temp2, username) == 0) {
        char* user_socket;
        user_socket = strtok(NULL, ":");
        return atoi(user_socket);
      }
    }
    bzero((char *)&temp, sizeof(temp));
  }
  return -1;
}

static long getMicrotime() {
  // TODO: change this to a formatted time string rather than a long
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}


char * getCurrentTime() {
  time_t rawtime;
  struct tm * timeinfo;

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  char * timestring = malloc(strlen(asctime(timeinfo)));

  strcpy(timestring, asctime(timeinfo));

  // Remove newline
  timestring[strlen(timestring) - 1] = '\0';

  return timestring;
  // TODO: free all versions of this

}

char * find_username(int socket){
  FILE *fp = fopen("Clients.txt", "r");
  char * username = malloc(sizeof(char)*(BUFFER_MAX_SIZE)); // TODO enter some fucking frees()

  char temp[BUFFER_MAX_SIZE];
  bzero((char *)&temp, sizeof(temp));
  while(fgets(temp, sizeof(temp), fp)) {
    char* loop_username;
    char* loop_socket;
    bzero((char *)&loop_username, sizeof(loop_username));
    loop_username = strtok(temp, ":");
    loop_socket = strtok(NULL, ":");
    loop_socket[strlen(loop_socket) - 1] = '\0';
    if (atoi(loop_socket) == socket) {
      sprintf(username, "%s", loop_username);
      break;
    }
    bzero((char *)&temp, sizeof(temp));
  }
  fclose(fp);

  return username;
}

int history(int from, int to, char* message){
  // generate timestamp
  long t = getMicrotime();
  char * datetime = getCurrentTime();
  char entry[BUFFER_MAX_SIZE];

  // get username from sender socket
  char* from_username = find_username(from);

  if(to == -1){
    debug("Got to broadcast entry\n");
    sprintf(entry, "%s\tBroadcasted from %s: %s\n", datetime, from_username, message);
    //write entry to each person's file
    FILE* fp_reading = fopen("Clients.txt", "r");
    char temp[BUFFER_MAX_SIZE];
    bzero((char *)&temp, sizeof(temp));
    while(fgets(temp, sizeof(temp), fp_reading)) {
      if (strcmp(temp, "") != 0) {
        char* user = strtok(temp, ":");
        printf("Got to user %s in clients file\n", user);
        char file_name[BUFFER_MAX_SIZE];
        bzero((char *)&file_name, sizeof(file_name));
        sprintf(file_name, "%s.chat", user);
        FILE *fp_user = fopen(file_name, "a");
        printf("The file pointer for user file %s: %d\n", user, fp_user);
        fputs(entry, fp_user);
        fclose(fp_user);
      }
      bzero((char *)&temp, sizeof(temp));
    }
    fclose(fp_reading);

  } else {

    // find the 'to' user
    char *to_username = find_username(to);

    sprintf(entry, "%s\tPrivate message from %s to %s: %s\n", datetime, to_username, from_username, message);

    // put in history of sender
    char file_name[BUFFER_MAX_SIZE];
    bzero((char *)&file_name, sizeof(file_name));
    sprintf(file_name, "%s.chat", from_username);
    FILE *fp = fopen(file_name, "a");
    fputs(entry, fp);
    fclose(fp);

    //put in history of receiver
    bzero(file_name, sizeof(file_name));
    sprintf(file_name, "%s.chat", to_username);
    FILE *fp2 = fopen(file_name, "a");
    fputs(entry, fp);
    fclose(fp2);

  }

  return 1;

}

int broadcast(int clientSocket, char* received_message, FILE* fp) {
  char message_to_send[BUFFER_MAX_SIZE];
  bzero((char*)&message_to_send, sizeof(message_to_send));
  sprintf(message_to_send, "%s", "*** Incoming Public Message ***: ");
  strcat(message_to_send, received_message);

  char temp[BUFFER_MAX_SIZE];
	bzero((char *)&temp, sizeof(temp));
	while(fgets(temp, sizeof(temp), fp)){
    if (strcmp(temp, "") != 0){
			char* file_socket;
      char* username;
			bzero((char *)&file_socket, sizeof(file_socket));
      bzero((char *)&username, sizeof(username));
			username = strtok(temp, ":");
      file_socket = strtok(NULL, ":");
			int user_socket = atoi(file_socket);
			if (user_socket != clientSocket) {
				send_string(user_socket, message_to_send);
			}
		}
		bzero((char *)&temp, sizeof(temp));
  }

  return 1;
}

void* connection_handler(void* socket) {
    int clientSocket = *(int*)socket;
    int rec;
    debug("About to try to recieve string\n");

    // Receive username

    char *username = receive_string(clientSocket);
    //username[strlen(username) - 1] = '\0';
    printf("username: %s\n", username);

    // Check if the user exists already

    FILE *fp = fopen("Users.txt", "r");
    int found = search(fp, username);
    printf("Status of found: %d\n", found);
    fclose(fp);

    send_int(clientSocket, found);

    if (found) { // user exists
      // receive password
      char *password = receive_string(clientSocket);
      //password[strlen(password) - 1] = '\0';

      // check if password if correct
      fp =fopen("Users.txt", "r");
      int status = check_password(fp, username, password);
      printf("Status of password check: %d", status);
      fclose(fp);

      // send status of if password was correct
      send_int(clientSocket, status);

      //Add user to Clients file of currently signed-on users
      if (status == 1) {
        FILE* fp = fopen("Clients.txt", "a");
        int response = new_client(fp, username, clientSocket);
        fclose(fp);
        if (response == 0) {
          printf("Adding to clients file failed.\n");
        }
        else {
          printf("Adding to clients file successful!\n");
        }
      }

    } else { // user DNE
      // receive new password
      char *password = receive_string(clientSocket);
      //password[strlen(password) - 1] = '\0';

      printf("Password received: %s\n", password);

      // write new user/password to the file
      fp = fopen("Users.txt", "a");
      int status = new_user(fp, username, password);
      fclose(fp);

      debug("Wrote new user to the file\n");

      // send status of creating new user
      send_int(clientSocket, status);

      //Add user to Clients file of currently signed-on users
      if (status == 1) {
        FILE* fp = fopen("Clients.txt", "a");
        int response = new_client(fp, username, clientSocket);
        fclose(fp);
        if (response == 0) {
          printf("Adding to clients file failed.\n");
        }
        else {
          printf("Adding to clients file successful!\n");
        }
      }

      debug("Sent status\n");

    }

    //-------
    //Receive command from client
    //char* option = "";
    while (1) {
      enum Operation option = receive_int(clientSocket);
      //BROADCAST
      if (option == BROADCAST) {
        // TODO: should we broadcast if no is online?
        printf("in broadcast option!\n");
        send_string(clientSocket, "ACK");
        char* received_message = receive_string(clientSocket);
        //printf("%s\n", received_message);
        FILE* fp = fopen("Clients.txt", "r");
        int status = broadcast(clientSocket, received_message, fp);
        fclose(fp);
        debug("Got out of broadcast function\n");
        if(status == 1){
          // add history entry
          int status_hist = history(clientSocket, -1, received_message);

          // send success
          send_string(clientSocket, "CONF_SUCCESS");
        } else {
          send_string(clientSocket, "CONF_FAIL");
        }

      }

      //PRIVATE
      else if (option == PRIVATE) {
        printf("in private option!\n");
        //send_string(clientSocket, "ACK");

        // TODO: need to format the message. currently sending it raw

        //Send client list of active users
        FILE* fp = fopen("Clients.txt", "r");
        char username_list[BUFFER_MAX_SIZE];
        bzero(username_list, sizeof(username_list));

        char temp[BUFFER_MAX_SIZE];
      	bzero((char *)&temp, sizeof(temp));
        int count = 1;
      	while(fgets(temp, sizeof(temp), fp)){
          if (strcmp(temp, "") != 0){
            char* user;
            bzero((char *)&user, sizeof(user));
      			user = strtok(temp, ":");
      			if (strcmp(username, user) != 0) {
              char otherTemp[BUFFER_MAX_SIZE];
              bzero(otherTemp, sizeof(otherTemp));
              sprintf(otherTemp, "%d) %s\n", count, user);
              strcat(username_list, otherTemp);
              count++;
      			}
      		}
      		bzero((char *)&temp, sizeof(temp));
        }
        fclose(fp);

        if(strlen(username_list) <= 0){ // there are no other users
          send_string(clientSocket, "CONF_FAIL");
        } else {
          send_string(clientSocket, "CONF_SUCCESS");

          send_string(clientSocket, username_list);

          char* received_username = receive_string(clientSocket);
          char* received_message = receive_string(clientSocket);

          FILE* fp2 = fopen("Clients.txt", "r");
          int userSocket = find_user_socket(fp2, received_username);
          fclose(fp2);
          if (userSocket == -1) {
            send_string(clientSocket, "CONF_FAIL");
          } else {
            send_string(userSocket, received_message);
            send_string(clientSocket, "CONF_SUCCESS");

            history(clientSocket, userSocket, received_message);
          }
        }
        printf("leaving private func\n");
      }

      // HISTORY bookmark
      else if (option == HISTORY) {
        char *user = find_username(clientSocket); // TODO: FREE
        printf("Determined user to be %s\n", user);
        char filename[BUFFER_MAX_SIZE];
        bzero((char *)&filename, sizeof(filename));
        sprintf(filename, "%s.chat", user);
        printf("File is %s\n", filename);
        FILE* fp_hist = fopen(filename, "r");
        printf("Opened file^\n");

        char hist_file_contents[BUFFER_MAX_SIZE];
        char line[BUFFER_MAX_SIZE];
        char temp[BUFFER_MAX_SIZE];

        bzero((char *)&hist_file_contents, sizeof(hist_file_contents));
        bzero((char *)&line, sizeof(line));
        bzero((char *)&temp, sizeof(temp));
        printf("Initialized and bzerod everyone\n");

        while (fgets(temp, sizeof(temp), fp_hist)) {
            //printf("In while loop\n");
            sprintf(line, "%s", temp);
            strcat(hist_file_contents, line);
            bzero((char *)&temp, sizeof(temp));
            bzero((char *)&line, sizeof(line));
        }

        // printf("hist file contents: %s\n", hist_file_contents);
        send_string(clientSocket, hist_file_contents);

        send_string(clientSocket, "ACK");

        fclose(fp_hist);

        printf("leaving history function\n");
      }

      // EXIT
      else if (option == EXIT) {
          // Respond to client
          send_string(clientSocket, "EXIT");

          // Closes socket
          printf("Closing socket with client\n");
          close(clientSocket);
          // Remove as a part of online client

          // Renames file
          printf("renaming file\n");

          rename("Clients.txt", "tempClients.txt");

          // Opens both files
          printf("opening files\n");
          FILE* f_in  = fopen("tempClients", "r");
          FILE* f_out = fopen("Clients.txt", "a");
          // Clients.txt is opened as appending so in case another thread
          // created the file, this thread doesn’t overwrite it

          // Go through each line of the file
          printf("going through each line\n");
          char buffer[BUFFER_MAX_SIZE];

          while (fgets(buffer, sizeof(buffer), f_in)) {
            // Extract socket from line of file
            char * username = strtok(buffer, ":");
            char * file_socket = strtok(NULL, ":");
            int user_socket = atoi(file_socket);
            // If the socket isnt the one we are searching for, add it to the file
            if (user_socket != clientSocket) {
                fputs(buffer, f_out);
            }
          }

          fclose(f_in);
          fclose(f_out);
          remove("tempClients.txt"); // TODO: this is not working. something with exit is weird
          break;

      }


    }

    // make sure this happens after we finish using username
    free(username);


}
