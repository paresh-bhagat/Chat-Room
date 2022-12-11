#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#define MAX_CLIENTS 20
#define BUFFER_SIZE 200
#define MODULO 128
#define NAME_SIZE 20

// error

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// generate private key

uint8_t generate_private_key()
{
    srand(time(0));
    uint8_t private_key = rand()%MODULO;
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

// Client structure 

typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
    uint8_t public_key;
	char name[NAME_SIZE];
} client_t;

// global variables

client_t *clients[MAX_CLIENTS];
static _Atomic unsigned int cli_count = 0;
static int uid = 10;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// print ip address

void print_client_addr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d",
         addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// remove \n from string

void string_trimln(char* message) 
{
    int i;
    for (i = 0; i < strlen(message); i++) 
    { // trim \n
        if (message[i] == '\n') 
        {
            message[i] = '\0';
            break;
        }
    }
}

// add client to queue

void queue_add(client_t *client_structure)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
    {
		if(!clients[i])
        {
			clients[i] = client_structure;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Remove clients from queue

void queue_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
    {
		if(clients[i])
        {
			if(clients[i]->uid == uid)
            {
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Send message to all clients except sender 

void send_message(char *message, int uid)
{
    int n;
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i)
    {
		if(clients[i])
        {
			if(clients[i]->uid != uid)
            {
				n = write(clients[i]->sockfd,message,strlen(message));
        	    if (n < 0)
            	    printf("ERROR writing to socket to user id=%d",uid);
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Handle all communication with the client 

void *handle_client(void *arg)
{
	char buffer[BUFFER_SIZE];
	char name[NAME_SIZE];
    int n;

	cli_count++;
	client_t *client_structure = (client_t *)arg;

	// Read name of client

    n = read( client_structure->sockfd, name , NAME_SIZE);
    //decrypt(private_key,client_name); 
    if (n < 0)
        error("ERROR reading from socket");
	
    string_trimln(name);

	strcpy(client_structure->name, name);
	sprintf(buffer, "%s has joined", client_structure->name);
	printf("\n%s", buffer );
	send_message(buffer, client_structure->uid);

	while(1)
    {
        bzero(buffer,BUFFER_SIZE);  

		n = read(client_structure->sockfd , buffer, BUFFER_SIZE );
        if (n < 0)
    		error("ERROR writing to socket");

        int e = strncmp( "exit",buffer,4);

        if( strlen(buffer) > 0 )
        {
            send_message(buffer, client_structure->uid);
            printf("%s\n", buffer);
        }
		else if ( e == 0 ) 
        {
			sprintf(buffer, "%s has left", client_structure->name);
			printf("\n%s", buffer );
			send_message(buffer, client_structure->uid);
		} 
	}

    // Delete client from queue and yield thread

    close(client_structure->sockfd);
    queue_remove(client_structure->uid);
    free(client_structure);
    cli_count--;
    pthread_detach(pthread_self());

	return NULL;
}

// main funtion

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno,temp;
     socklen_t clilen;
     char buffer[100],server_name[20],client_name[20];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     pthread_t tid;

     if (argc < 2) 
     {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);

     //build server address structure

     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     serv_addr.sin_port = htons(portno);

     //bind local port number

     if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR on binding");

     //specify number of concurrent
     //clients to listen for

     listen(sockfd,10);

     printf(" Chat room started \n");

     while(1)
     {
    
     	//Wait for client

     	clilen = sizeof(cli_addr);
     	newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
     	if (newsockfd < 0)
        	error("ERROR on accept");
     	else
        	printf("Received connection from host [IP %s TCP port %d] \n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));

        if((cli_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			close(newsockfd);
			continue;
		}

        client_t *client_structure = (client_t *)malloc(sizeof(client_t));
		client_structure->address = cli_addr;
		client_structure->sockfd = newsockfd;
		client_structure->uid = uid++;

		// Add client to the queue and fork thread 

		queue_add(client_structure);
		pthread_create(&tid, NULL, &handle_client, (void*)client_structure);

		/* Reduce CPU usage */
		sleep(1);
	}

    return 0;
}