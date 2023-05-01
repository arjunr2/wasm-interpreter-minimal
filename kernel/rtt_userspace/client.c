// Client side implementation of UDP client-server model
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>

#define MAXLINE 1024

char* server_ip="192.168.1.216";
unsigned short port = 8008;

uint64_t getustime() {
	struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)(ts.tv_sec * 1000000) + (uint64_t)(ts.tv_nsec / 1000);
}

// Driver code
int main() {
	int sockfd;
	char buffer[MAXLINE];
	const char *hello = "Hello from client";
	struct sockaddr_in	 servaddr;

	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(server_ip);
	
	int n;
	socklen_t len;
	
	for (int i = 0; i < 3000; i++) {
		uint64_t start_time = getustime();
		sendto(sockfd, (const char *)hello, strlen(hello),
			MSG_CONFIRM, (const struct sockaddr *) &servaddr,
				sizeof(servaddr));
			
		n = recvfrom(sockfd, (char *)buffer, MAXLINE,
					MSG_WAITALL, (struct sockaddr *) &servaddr,
					&len);
		uint64_t end_time = getustime();
		uint64_t time_diff = end_time - start_time;
		printf("RTT Time:%lu\n", time_diff);
		usleep(5000);
	}

	close(sockfd);
	return 0;
}

