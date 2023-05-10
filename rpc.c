#include <netdb.h> 
#include <string.h> 
#include <stdio.h> 
#include <unistd.h> 
#include <stdlib.h>
#include "rpc.h"
#include "utils.h"

struct rpc_server {
    struct sockaddr_in* address;
    int sockfd;
};

rpc_server *rpc_init_server(int port) {
    // Initialize socket
    char buffer[2048];
    struct sockaddr_in address;
    int sockfd, newsockfd;
    int n;

    sockfd = setup_server_socket(port, &address);

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
        n = read(newsockfd, buffer, 2047);
        if (n == 0) {
            close(newsockfd);
            continue;
        }
        if (n < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        buffer[n] = '\0';

        printf("%s\n", buffer);

        // TODO: perform different actions depending on input from client

        close(newsockfd);
    }

    rpc_server server = {
        .address = &address,
        .sockfd = sockfd
    };

    return &server;
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
    struct sockaddr_in server_address;
    int sockfd;
    char buffer[256];

    while (1) {
        // Make connection
        sockfd = setup_client_socket(port, &server_address);
        if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) <0) {
            perror("connect");
            exit(EXIT_FAILURE);
        }

        printf("Enter command: ");
        if (!fgets(buffer, 255, stdin)) {
            break;
        }
        strtok(buffer, "\n");

        // TODO: validate input based on type of action

        close(sockfd);
    }

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
