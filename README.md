# Programming Assignment 03
### Bailey Blum, Catherine Markley, Chris Foley

Included File Names:
communications.h
parameters.h
chat/chatclient.c
chat/Makefile
server/chatserver.c
server/logs/Users.txt
server/logs/Clients.txt
server/Makefile

To run our code, we just step by step followed the demo video. The Users.txt file contains the list of all registered users, and Clients.txt contains all users that are currently online. The logs folder contains all of the chat histories of each user. Outside of the client and server directories are two header files containing widely used functions.

To start our server, just type make and then ./chatserver 41002. To run our client, just type make and then ./chatclient student12.cse.nd.edu 41002 Alice. Make sure you make within the chat or server directory. From this, just follow the example video for exact commands to run.
