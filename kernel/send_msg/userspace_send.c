#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define SERVER_PORT 8008
#define SERVER_IP "127.0.0.1"

int main(int argc, char *argv[]) {

    // Create a socket for sending UDP packets
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the destination address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &dest_addr.sin_addr);

    // Send UDP packets every 0.5 seconds
    while (1) {
        char message[] = "Hello, world!";
        if (sendto(sock, message, sizeof(message), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto failed");
        }
        sleep(1); // wait for 0.5 seconds
    }

    // Close the socket
    close(sock);

    return 0;
}

