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

// #define SERVER_ADDR "127.0.1.1"
#define modulo 128

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// generate private key

uint8_t generate_private_key()
{
    srand(time(0));
    uint8_t private_key = rand()%modulo;
    return private_key;
}

// generate public key

uint8_t generate_public_key(uint8_t private_key)
{
    uint8_t public_key = modulo - private_key;
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
        char c = (temp[i] + public_key) % modulo;
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
        char c = (message[i] + private_key) % modulo;
        message[i]=c;
    }
    //printf("message after decryption=%s\n",message);
}

// main funtion

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno,temp;
     socklen_t clilen;
     char buffer[100],server_name[20],client_name[20];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
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

     listen(sockfd,5);
     while(1)
     {
     	printf("\nWaiting for a new client connection on [TCP port %d]\n",portno);

     	//Wait for client

     	clilen = sizeof(cli_addr);
     	newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
     	if (newsockfd < 0)
        	error("ERROR on accept");
     	else
        	printf("Received connection from host [IP %s TCP port %d] \n",inet_ntoa(cli_addr.sin_addr),ntohs(cli_addr.sin_port));

        // generate private key

        uint8_t private_key= generate_private_key();

        // generate public key

        uint8_t public_key = generate_public_key(private_key);

        // get public key from client

        uint8_t public_key_client;
        n = read(newsockfd,&public_key_client,sizeof(uint8_t));
        if (n < 0)
            error("ERROR writing to socket");

        // send public key to client

        n = write(newsockfd,&public_key,sizeof(uint8_t));
        if (n < 0)
            error("ERROR writing to socket");

        // read client name

     	n=read(newsockfd,client_name,20);
        decrypt(private_key,client_name); 
     	if (n < 0)
            error("ERROR reading from socket");
        
        // enter username

        printf("Enter your username : ");
    	fgets(server_name,20,stdin);
    	char* first_newline = strchr(server_name, '\n');
    	if (first_newline)
      		*first_newline = '\0';
      		
        // send name to client
        char *encrypted_message;
        encrypted_message = encrypt(public_key_client,server_name);
        n=write(newsockfd,encrypted_message,strlen(encrypted_message));
        if (n < 0)
            	error("ERROR writing to socket");
                  
        //send and receive data from client
        
        
        printf("\nStart chat...\n");
        while(1)    
        {
            // clean buffer

        	bzero(buffer,100);

            // read message from client and decrpyt

        	n=read(newsockfd,buffer,100);
        	if (n < 0)
            	error("ERROR reading from socket");
            decrypt(private_key,buffer); 

            // print on screen
            printf("%s : %s\n",client_name,buffer);
            bzero(buffer,100);

            // type messsage
            printf("%s : ",server_name);
            fgets(buffer,100,stdin);
            char* first_newline = strchr(buffer, '\n');
            if (first_newline)
                *first_newline = '\0';

            // send message to client
            encrypted_message = encrypt(public_key_client,buffer);
        	n=write(newsockfd,encrypted_message,strlen(encrypted_message));
        	if (n < 0)
            	error("ERROR writing to socket");

            // check for condition for exit
            int i = strncmp("Bye",buffer,3);
        	if(i==0)
        	{
        		close(newsockfd);
                       printf("Client disconnected\n");
            		break;
            	}
        }
     }
     
     return 0;
}