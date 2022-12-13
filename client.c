#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 200
#define MODULO 128
#define NAME_SIZE 20

//error

void error(const char *msg)
{
    perror(msg);
    printf("closing connection with the server\n");
    exit(0);
}

// remove  from string

void string_trimln(char* message) 
{
    char* first_newline = strchr(message, '\n');
    if (first_newline)
        *first_newline = '\0';
}

// generate private key

uint8_t generate_private_key()
{
    srand(time(0));
    uint8_t private_key = rand() % MODULO;
    return private_key;
}

// generate public key

uint8_t generate_public_key(uint8_t private_key)
{
    uint8_t public_key = MODULO - private_key;
    return public_key;
}

// Encrypt function for messages

char* encrypt(uint8_t public_key,char* message)
{
    //printf("original message=%s\n",message);
    int n = strlen(message);
    char* temp = (char *)malloc(n+1);
    strcpy(temp, message);

    for (int i=0;i<n;i++){
        char c = (temp[i] + public_key) % MODULO;
        temp[i]=c;
    }
    //printf("Encrypted message=%s\n",temp);
    return (char *)temp;
}

// Decrypt funtion for messsages

void decrypt(uint8_t private_key,char* message)
{
    //printf("message to de deccrypted=%s\n",message);
    int n = strlen(message);

    for (int i=0;i<n;i++){
        char c = (message[i] + private_key) % MODULO;
        message[i]=c;
    }
    //printf("message after decryption=%s\n",message);
}

// global variables

char name[NAME_SIZE];
int sockfd = 0;
pthread_t tmp_thread;

// function for sending message
void str_overwrite_stdout() 
{
    printf("%s", ">");
    fflush(stdout);
}

void *send_message() 
{
    char message[BUFFER_SIZE];
	char buffer[BUFFER_SIZE + NAME_SIZE + 4];

    while(1) 
    {

        bzero(message,BUFFER_SIZE);
        bzero(buffer,BUFFER_SIZE + NAME_SIZE + 4 );
        str_overwrite_stdout();
        fgets(message,BUFFER_SIZE, stdin);
        string_trimln(message);

        int i = strncmp("exit",message,4);
        if(i==0)
        {
            write(sockfd,message,strlen(message));
            pthread_cancel(tmp_thread);
            pthread_exit(NULL);
            break;
        }
            

        sprintf(buffer, "%s : %s", name, message);
        //printf("\nEntered message = %s",buffer);
        write(sockfd,buffer,strlen(buffer));
    }
}

// function for receiving message

void *recieve_message() 
{
	char message[BUFFER_SIZE];
    tmp_thread = pthread_self();
    while (1) 
    {
        bzero(message, BUFFER_SIZE);

        int n = read( sockfd, message, BUFFER_SIZE );
        if (n < 0)
            error("ERROR reading from socket"); 

        //printf("Message received");
        printf("%s\n", message); 
        str_overwrite_stdout();
    }
}

// main funtion

int main(int argc, char *argv[])
{
    int portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[100];
    char end[]="end connection";
    if (argc < 3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);

    // create stream socket

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);

    if (server == NULL) 
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    //build server address structure

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    //connect to server

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    else{printf("\nEntered Chat Room successfully\n");}

    // enter your user name

    printf("Enter your username : ");
    fgets(name,NAME_SIZE,stdin);
    string_trimln(name);
    
    n = write(sockfd,name,strlen(name));
    if (n < 0)
    	error("ERROR: writing to socket");

	pthread_t send_message_thread;
    pthread_t recieve_message_thread;

    pthread_create(&send_message_thread, NULL, &send_message, NULL);
    pthread_create(&recieve_message_thread, NULL, &recieve_message, NULL);

    pthread_join(send_message_thread,NULL);
    pthread_join(recieve_message_thread,NULL);

	close(sockfd);

	return 0;
}

/*
    // generate private key

    uint8_t private_key= generate_private_key();

    // generate public key

    uint8_t public_key = generate_public_key(private_key);

    // send public key to server

    n = write(sockfd,&public_key,sizeof(uint8_t));
    if (n < 0)
        error("ERROR writing to socket");

    // get public key from server

    uint8_t public_key_server;
    n = read(sockfd,&public_key_server,sizeof(uint8_t));
    if (n < 0)
        error("ERROR writing to socket");
    
    // send username to server

    char *encrypted_message;
    encrypted_message = encrypt(public_key_server,client_name);
    n = write(sockfd,encrypted_message,strlen(encrypted_message));
    if (n < 0)
    	error("ERROR writing to socket");
    
    // read server name

    n = read(sockfd, server_name, 20);
    decrypt(private_key,server_name); 
    if (n < 0)
        error("ERROR reading from socket"); 
    printf("\nStart chat...\n");
    
    // send and receive data from server

    while(1)
    {
        // clean buffer

    	bzero(buffer,100);

        // print on screen and type messsage

    	printf("%s : ",client_name);
    	fgets(buffer,100,stdin);
        char* first_newline = strchr(buffer, '\n');
        if (first_newline)
            *first_newline = '\0';

        // send message to server
        char *encrypted_message;
        encrypted_message = encrypt(public_key_server,buffer);
    	n=write(sockfd,encrypted_message,strlen(encrypted_message));
    	if (n < 0)
    		error("ERROR writing to socket");

        // check for condition for exit
    	int i = strncmp("Bye",buffer,3);
        if(i==0)
            break;
        
        // read message from server and decrpyt
    	bzero(buffer,100);
     	n=read(sockfd,buffer,100);
     	if (n < 0)
        	error("ERROR reading from socket"); 
        decrypt(private_key,buffer); 

        // print on screen

        printf("%s : %s\n",server_name, buffer);
        
    }  
    printf("\nClosing connection from Server\n");
    close(sockfd);
    return 0;
}
*/