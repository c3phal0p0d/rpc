/* Socket-related functions adapted from workshop 9 code */
#define _POSIX_C_SOURCE 200112L
#include <netdb.h> 
#include <strings.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include "utils.h"

/* Create and return a server socket bound to the given port */
int setup_server_socket(const int port, struct sockaddr_in* address) {
    int sockfd;

    // Create socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Create listen address for given port number for all IP addresses of this machine
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(port);

    // Reuse port if possible
    int re = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
        perror("Could not reopen socket");
        exit(EXIT_FAILURE);
    }

    // Bind address to socket
    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

/* Create and return a client socket bound to the given port and server */
int setup_client_socket(const int port, struct sockaddr_in* server_address) {
    int sockfd;

    // Create socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}