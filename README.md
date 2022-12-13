# Chat Room

A multiple client Chat Room application using Socket Programming in C. It uses concept of multithreading in C using pthread library to handle multiple clients. The server and multiple client processes can run on same or different machines. Multiple client processes can connect to the same server and send messages in the chat room. Messages are end-to-end encrypted using a simple algorithm for generating public key,private key pair.

## Tags
C programming, Socket programming, Multithreading in C using pthread Library, E2EE

## Demo

To compile server.c and client.c

```
gcc server.c -o server -lpthread
gcc client.c -o client -lpthread
```

To start server, PORT number is to be given as argument(ex. 9999)

```
./server PORT_NUMBER
````

To execute the client process, the IP address and PORT number of server is to be passed as command line argument.
```
./client IP_ADDR_OF_SERVER PORT_NUMBER
```

Choose a username of your choice and start sending messages in the chatroom.
