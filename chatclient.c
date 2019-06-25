//#Connie McClung
//CS372 Fall 2018
//Project 1
//chatclient.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h>//sockaddr_in structure
#include <sys/socket.h>//socket functions
#include <arpa/inet.h>//in_addr structure

#define MAXHANDLE 10 // max size for client handle
#define MAXMSGSIZE 500 //max bytes allowed in message

//function declarations
void *get_in_addr(struct sockaddr *socketAddress);
char* getUserHandle();
char* getKeyboardInput(char* clientHandle);
int sendAllBytes(int socketfd, char* msgBuffer);
int makeConnection(char *host, char *port);
void handleComm(int socketfd, char *clientHandle);

//	get_in_addr()
//	function to get sockaddr, IPv4 or IPv6 from
//	https://beej.us/guide/bgnet/html/single/bgnet.html#ipaddrs2
void *get_in_addr(struct sockaddr *socketAddress)
{
	if (socketAddress->sa_family == AF_INET)//if IPv4, return correct struct 
	{
		return &(((struct sockaddr_in*)socketAddress)->sin_addr);
	}
	//else return struct for IPv6
	return &(((struct sockaddr_in6*)socketAddress)->sin6_addr);
}

//	getUserHandle()
//	function to get user input for client handle, adapted from my function in a CS344 assignment
char* getUserHandle()
{
	size_t bufferSize = MAXHANDLE; //maximum chars to read in
	char* lineEntered = malloc(bufferSize * sizeof(char)); //allocate memory
	memset(lineEntered, '\0', bufferSize); //initalize
	int numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
	if (numCharsEntered == -1)//error handling
	{
		perror("getline() failure\n");
		return NULL;
	}
	lineEntered[numCharsEntered-1] = '\0'; //strip newline and terminate the string
	return lineEntered; 
}

//	getKeyboardInput()
//	function to get keyboard input from user for messages, adapted from my function in a CS344 assignment
char* getKeyboardInput(char* clientHandle)
{
	size_t bufferSize = MAXMSGSIZE; //maximum chars to read in
	char* lineEntered = malloc(bufferSize * sizeof(char)); //allocate memory
	memset(lineEntered, '\0', bufferSize); //initalize
	printf("%s", clientHandle);fflush(stdout);//prompt for user to enter client message

	int numCharsEntered = getline(&lineEntered, &bufferSize, stdin);
	if (numCharsEntered == -1)
	{
		perror("getline() failure\n");
		return NULL;
	}
	lineEntered[numCharsEntered-1] = '\0'; //strip newline and terminate the string
	return lineEntered; 
}

//	sendAllBytes()
//	function to ensure all bytes in message are written to socket
//	based on sendall() function code at https://beej.us/guide/bgnet/html/multi/advanced.html
//	takes socket fd and string message and loops send until all bytes are sent or error is returned
int sendAllBytes(int socketfd, char* msgBuffer)
{
	int bytesSent;
	int totalBytesSent = 0;
	int bytesRemaining = strlen(msgBuffer);
	while (totalBytesSent < strlen(msgBuffer))
	{
		bytesSent = send(socketfd, msgBuffer + totalBytesSent, bytesRemaining, 0);
		if (bytesSent == -1)
		{
			perror("client: send failure");
			exit(1);
		}
		totalBytesSent += bytesSent;
		bytesRemaining -= bytesSent;
	}
	if (bytesSent == -1)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

//	makeConnection()
//	function to resolve commandline hostname to ip address and connect to server socket at specified port
//	uses code and examples explained at https://beej.us/guide/bgnet/html/multi/clientserver.html
int makeConnection(char *host, char *port)
{
	int socketfd;
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];
	int returnValue;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((returnValue = getaddrinfo(host,port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(returnValue));
		return 1;
	}

	//create socket
	for (p = servinfo; p !=NULL; p = p->ai_next)
	{
		if ((socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client:socket creation error");
			continue;
		}

		//connect to server
		//printf("Socket created.\n");fflush(stdout);

		if (connect(socketfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(socketfd);
			perror("client:connect error");
			continue;
		}

		break;
	}
	//error handling
	if (p == NULL)
	{
		fprintf(stderr, "Client: failed to connect \n");
		return 2;
	}
	
	
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s,sizeof s);
	printf("\nClient is now connected to server at %s...\n", s);fflush(stdout);
	freeaddrinfo(servinfo);
	return socketfd;
}

//handleComm()
//function to send/receive messages and process quit request from server or client
void handleComm(int socketfd, char *clientHandle)
{
	//printf("\nI'm in handleComm function now\n");fflush(stdout);
	int bytesSent;
	int bytesReceived;

	char clientMsg[MAXMSGSIZE];
	char serverReply[MAXMSGSIZE];

	memset(clientMsg, '\0', MAXMSGSIZE); //initalize
	memset(serverReply, '\0', MAXMSGSIZE); //initalize

	//receive from server	
	bytesReceived = recv(socketfd, serverReply, MAXMSGSIZE - 1, 0);
	//printf("\nbytes received from server: %d\n", bytesReceived);fflush(stdout);
	//check for errors
	if (bytesReceived == -1)
	{
		perror("receive error");
		exit(1);
	}
	serverReply[bytesReceived] ='\0';//null terminate received message string
	if (strstr(serverReply, "\\quit") != NULL)//if server sends quit request, close socket and exit program
	{
		printf("Server sent quit request, terminating client program...\n");fflush(stdout);
		close(socketfd);
		exit(0);
	}
	printf("%s\n", serverReply);fflush(stdout);

	//send message to server
	//get client keyboard input message and append to handle, send to server
	strcpy(clientMsg, clientHandle);
	char *clientInput = getKeyboardInput(clientHandle);
	strcat(clientMsg, clientInput);
	//error checking
	if ((bytesSent = sendAllBytes(socketfd, clientMsg)) == -1)
	{
		perror("send error");
		exit(1);
	}
	if (strcmp(clientInput, "\\quit") == 0)//if client sends quit request, send it, close socket  and exit program
	{
		printf("Client sent quit request to server, terminating client program...\n");fflush(stdout);
		close(socketfd);
		exit(0);
	}
}



int main(int argc, char *argv[])
{
	int socketfd;
	int bytesReceived;

 	char* clientHandle;

	//check for correct number of arguments
	if (argc != 3) { fprintf(stderr,"USAGE: %s <server-hostname> <port#>\n", argv[0]); exit(0); } 
	
	//get client handle
	printf("\nPlease enter client handle (10 char max): ");fflush(stdout);	
	clientHandle = getUserHandle();
	
	//get handle and construct initial message to server
	char* initialMessage= malloc(MAXMSGSIZE * sizeof(char)); //allocate memory
	memset(initialMessage, '\0', MAXMSGSIZE); //initalize
	strcpy(initialMessage, clientHandle);
	strcat(initialMessage, "> ");
	strcat(initialMessage, clientHandle);
	strcat(initialMessage, " requests connection to server");
	
	//connect  to server at specified port
	socketfd = makeConnection(argv[1], argv[2]);

	//send initial message to server
	if (sendAllBytes(socketfd, initialMessage) == -1)
	{
		perror("send error");
	}
	//append > to handle
        strcat(clientHandle, "> ");

	printf("\nEnter your messages when client prompt \"%s\" appears\n", clientHandle);fflush(stdout);
	printf("\nTo stop chatting, type \"\\quit\"\n\n");fflush(stdout);

	//chat with server until server or client sends quit request
	while(1)
	{
		handleComm(socketfd, clientHandle);
	}
	
	close(socketfd);
	exit(0);
}	
