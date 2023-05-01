// Server side implementation of UDP client-server model
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>

#define MAXLINE 1024

char* server_ip="192.168.1.216";
unsigned short port = 8008;

// Driver code
int main() {
	int sockfd;
	char buffer[MAXLINE];
	const char *hello = "Hello world pong";
	struct sockaddr_in servaddr, cliaddr;
	
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
	
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = inet_addr(server_ip);
	servaddr.sin_port = htons(port);
	
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	
	socklen_t len;
int n;

	len = sizeof(cliaddr); //len is value/result

	int num_msgs = 0;
	while (1) {
		n = recvfrom(sockfd, (char *)buffer, MAXLINE,
					MSG_WAITALL, ( struct sockaddr *) &cliaddr,
					&len);
		sendto(sockfd, (const char *)hello, strlen(hello) + 1,
			MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
				len);
		//printf("Reply sent: %d\n", num_msgs++);
	}
	
	return 0;
}

