#define _POSIX_C_SOURCE 200112L
#include <netdb.h> 
#include <strings.h> 
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include "rpc.h"

struct rpc_server {
    struct sockaddr_in6* address;
    int sockfd;
};

int setup_server_socket(int port);
int setup_client_socket(char* hostname, const int port);

rpc_server *rpc_init_server(int port) {
    // Initialize socket
    int sockfd, newsockfd;

    sockfd = setup_server_socket(port);

    // Listen on socket for connections
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1){
        // Accept connection
        newsockfd = accept(sockfd, NULL, NULL);
        if (newsockfd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Read from client
        // TODO: perform different actions depending on input from client

        close(newsockfd);
    }

    printf("Server initiated\n");

    // rpc_server server = {
    //     .address = &address,
    //     .sockfd = sockfd
    // };

    return NULL;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    return -1;
}

void rpc_serve_all(rpc_server *srv) {

}

struct rpc_client {
    int sockfd;
};

struct rpc_handle {
    /* Add variable(s) for handle */
};

rpc_client *rpc_init_client(char *addr, int port) {
    int sockfd;

    sockfd = setup_client_socket(addr, port);
    printf("Client initiated\n");

	close(sockfd);

    return NULL;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    return NULL;
}

void rpc_close_client(rpc_client *cl) {

}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}


/* Adapted from workshop 9 code */
/* Create and return a server socket bound to the given port */
int setup_server_socket(const int port) {
    int re, s, sockfd;
	struct addrinfo hints, *res;

	// Create address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept

    // Get addrinfo
    char service[snprintf(NULL, 0, "%d", port) + 1];
    sprintf(service, "%d", port);
	s = getaddrinfo(NULL, service, &hints, &res);

	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
    
	freeaddrinfo(res);

    return sockfd;
}

/* Create and return a client socket bound to the given port and server */
int setup_client_socket(char* hostname, const int port) {
    int sockfd, s;
	struct addrinfo hints, *servinfo, *rp;

    // Create address
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;

	// Get addrinfo of server
    char service[snprintf(NULL, 0, "%d", port) + 1];
    sprintf(service, "%d", port);
    printf("hostname: %s\n", hostname);
	s = getaddrinfo(hostname, service, &hints, &servinfo);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Connect to first valid result
	for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1){
            printf("not valid\n");
            continue;
        }

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sockfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(servinfo);

    return sockfd;
}