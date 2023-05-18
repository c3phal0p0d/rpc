#define _POSIX_C_SOURCE 200112L
#include <netdb.h> 
#include <strings.h> 
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include "rpc.h"

typedef struct registered_function {
    int id;
    char name[10000];
    rpc_handler handler;
} registered_function;

struct rpc_server {
    int sockfd;
    int num_functions;
    registered_function **functions;
};

int validate_rpc_data(rpc_data *data){
    // Check that data2_len and actual length of data2 are the same
    //printf("validating rpc_data\n");
    if (data==NULL){
        //printf("data==NULL\n");
        return 0;
    }
    else if (data->data2_len<0){
        //printf("data2_len<0\n");
        return 0;
    }
    else if (data->data2_len>0 && data->data2==NULL){
        //printf("data2_len>0, data2==NULL\n");
        return 0;
    }
    else if (data->data2_len==0 && data->data2!=NULL){
        //printf("data2_len==0, data2!=NULL\n");
        return 0;
    }
    return 1;
    // int actual_data2_len = 0;
    // uint8_t *elem = data->data2;
    // printf("elem: %d\n", *elem);
    // while (*elem) {
    //     elem++;
    //     printf("elem: %d\n", *elem);
    //     actual_data2_len++;
    //     if (actual_data2_len > data->data2_len){
    //         printf("actual data2_len: %d\n", actual_data2_len);
    //         return 0;
    //     }
    // }
    // if (actual_data2_len<data->data2_len){
    //     printf("actual data2_len: %d\n", actual_data2_len);
    //     return 0;
    // }
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
	hints.ai_flags = AI_PASSIVE;

    // Get addrinfo
    //printf("port: %d\n", port);
    char service[snprintf(NULL, 0, "%d", port) + 1];
    sprintf(service, "%d", port);
    //printf("service: %s\n", service);
	s = getaddrinfo(NULL, service, &hints, &res);

	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("ERROR: socket");
		return -1;
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("ERROR: setsockopt");
		return -1;
	}
	// Bind address to the socket

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("ERROR: bind");
		return -1;
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
    //printf("port: %d\n", port);
    char service[snprintf(NULL, 0, "%d", port) + 1];
    sprintf(service, "%d", port);
    //printf("service: %s\n", service);
    //// printf("hostname: %s\n", hostname);
	s = getaddrinfo(hostname, service, &hints, &servinfo);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	// Connect to first valid result
	for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1){
            //printf("not valid\n");
            continue;
        }

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sockfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}

	freeaddrinfo(servinfo);

    return sockfd;
}

rpc_server *rpc_init_server(int port) {
    // Initialize socket
    int sockfd;

    if ((sockfd = setup_server_socket(port))==-1){
        return NULL;
    };

   // printf("Server initiated\n");

    rpc_server *server = malloc(sizeof(rpc_server));
    server->sockfd = sockfd;
    server->num_functions = 0;
    server->functions = malloc(sizeof(registered_function));

    return server;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    // Check for valid inputs
    if (name==NULL || srv==NULL || handler==NULL){
        return -1;
    }

    registered_function *function = malloc(sizeof(registered_function));
    function->id = srv->num_functions;
    strcpy(function->name, name); 
    function->handler = handler;

    // printf("num_functions: %d\n", srv->num_functions);

    srv->functions[srv->num_functions] = function;
    srv->num_functions++;

    //printf("Registered function name: %s, id:%d\n", function->name, function->id);
    // printf("num_functions: %d\n", srv->num_functions);
    // printf("srv->functions[srv->num_functions]->name: %s\n", srv->functions[function->id]->name);

    return function->id;
}

void rpc_serve_all(rpc_server *srv) {
   // printf("Serving all....\n");
    int sockfd, newsockfd, n;
    char buffer[256];
    sockfd = srv->sockfd;

    // Listen on socket for connections
    if (listen(sockfd, 5) < 0) {
        perror("ERROR: listen");
        exit(EXIT_FAILURE);
    }

    // Accept connection
    struct sockaddr_storage client_addr;
	socklen_t client_addr_size;
    client_addr_size = sizeof client_addr;
	newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
	if (newsockfd < 0) {
		perror("accept");
		exit(EXIT_FAILURE);
	}
    //// printf("Accepted connection\n");

    while (1){
        n = read(newsockfd, buffer, 255);

        if (n==0){
             continue;
        }

		if (n < 0) {
			perror("ERROR reading from socket");
			return;
		}
        buffer[n] = '\0';
        //printf("read in message %s from client\n", buffer);

        // Get input from client & send a response
        char response[256];

        int request_id = atoi(strtok(buffer, " "));
        //printf("request id: %d\n", request_id);

        char *request = strtok(NULL, " ");
        //printf("request: %s\n", request);

        // FIND -> find function on server
        if (strncmp("FIND", request, 4)==0){
            char *request_data = strtok(NULL, " ");
            //printf("request data: %s\n", request_data);

            //printf("FIND request received\n");
            //printf("srv->functions[0]->name: %s\n", srv->functions[0]->name);
            int function_found = 0;
            //printf("num_function: %d\n", srv->num_functions);
            for (int i=0; i<srv->num_functions; i++){
                //// printf("function name: %s\n", srv->functions[0]->name);
                if (srv->functions[i]!=NULL && strcmp(srv->functions[i]->name, request_data)==0){
                    // Get function id
                    //printf("found, function id: %d\n", srv->functions[i]->id);
                    //// printf("request: %d, function id: %d\n", request_id, srv->functions[i]->id);
                    sprintf(response, "%d OK %d", request_id, srv->functions[i]->id);
                    function_found = 1;
                    break;
                }
            }
            if (!function_found){
                //printf("function not found\n");
                sprintf(response, "%d ER", request_id);
            }
        }
        // CALL -> call function and return its result
        else if (strncmp("CALL", request, 4)==0){
            //printf("CALL request\n");

            // Convert request data to rpc_data struct
            int function_id = atoi(strtok(NULL, " "));
            //printf("function id: %d\n", function_id);
            int data1 = atoi(strtok(NULL, " "));
            //printf("data1: %d\n", data1);
            size_t data2_len = atoi(strtok(NULL, " "));
            //printf("data2_len: %ld\n", data2_len);

            // Parse data2 byte array string into appropriate format to be used by function
            //char *data2_array_str = strtok(NULL, " ");
            char data2_array_str[data2_len*2 + 1];
            strcpy(data2_array_str, strtok(NULL, " "));
            //printf("data2 array str: %s\n", data2_array_str);
            //uint8_t data2_array[data2_len];
            uint8_t *data2_array = malloc(sizeof(uint8_t) * (data2_len + 1));
            data2_array[0] = atoi(strtok(data2_array_str, ","));
            //printf("data2_array[0]: %c\n", data2_array[0]);
            for (int i=1; i<data2_len; i++){
                data2_array[i] = atoi(strtok(NULL, ","));
            }

            void *data2 = data2_array;

            //printf("data2: %d\n", data2);
            rpc_data *input = malloc(sizeof(rpc_data));
            input->data1 = data1;
            input->data2_len = data2_len;
            input->data2 = data2;

            // Call function
            rpc_handler handler = srv->functions[function_id]->handler;
            rpc_data *result = handler(input);

            // Validate result rpc_data
            if (validate_rpc_data(result)) {
                //printf("result rpc_data is valid\n");
                //printf("result data1: %d\n", result->data1);
                //printf("result data2_len: %ld\n", result->data2_len);
                // printf("result data2: %d\n", result->data2);

                // Convert data2 byte array to string format
                char result_data2_array_str[result->data2_len*2];
                strcpy(result_data2_array_str, "");
                char data2_elem[snprintf(NULL, 0, "%d,", 1) + 1];
                //strcpy(data2_array, *(uint8_t *)payload->data2);
                for (int i=0; i<result->data2_len; i++){
                    sprintf(data2_elem, "%d,", *(uint8_t *)(result->data2+i));
                    strcat(result_data2_array_str, data2_elem);
                }
                //printf("result data2 array string: %s\n", result_data2_array_str);
                
                sprintf(response, "%d OK %d %d %ld %s", request_id, function_id, result->data1, result->data2_len, result_data2_array_str);

            } else {
                //printf("invalid input rpc_data\n");
                sprintf(response, "%d ER", request_id);
            }

            rpc_data_free(result);
            input->data2 = NULL;
            rpc_data_free(input);
        }

        // Send response to client
        //printf("Sent response %s\n", response);
        n = write(newsockfd, response, strlen(response));
		if (n < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}

    // close(newsockfd);
    }

    close(newsockfd);
}

struct rpc_client {
    int sockfd;
    int request_count;
};

struct rpc_handle {
    int function_id;
};

rpc_client *rpc_init_client(char *addr, int port) {
    int sockfd;

    if ((sockfd = setup_client_socket(addr, port))==-1){
        return NULL;
    };
    //// printf("Client initiated\n");

    rpc_client *client = malloc(sizeof(rpc_client));
    client->sockfd = sockfd;
    client->request_count = 0;

    return client;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    //printf("rpc_find called\n");
    int n, request_id;
    char request[256];
    char buffer[256];

    // Make sure inputs are not null
    if (name==NULL || cl == NULL){
        return NULL;
    }

    // Send FIND request to server: single message of form "request_id FIND name"
    request_id = cl->request_count;
    cl->request_count++;
    sprintf(request, "%d FIND %s", request_id, name);

    //printf("sending request: %s\n", request);

    n = write(cl->sockfd, request, strlen(request));
	if (n < 0) {
		perror("socket");
		return NULL;
	}
    //// printf("FIND request sent\n");

    // Get response from server of form "request_id OK function_id"
    //// printf("reading response from server\n");
    n = read(cl->sockfd, buffer, 255);
    buffer[n] = '\0';
    //printf("response from server: %s\n", buffer);

    // Check that ID of response corresponds to ID of request
    int response_id = atoi(strtok(buffer, " "));
    //printf("request id: %d\n", response_id);
    if (response_id!=request_id) {
        return NULL;
    }

    // Check if function was found
    char ok_message[3];
    strncpy(ok_message, buffer + 2, 2);
    ok_message[2] = '\0';
    //printf("ok message: %s\n", ok_message);

    if (strcmp(ok_message, "OK")==0){
        // Get function id from response
        char function_id_str[3];
        strncpy(function_id_str, buffer + 5, n-5);
        function_id_str[n-5] = '\0';
        int function_id = atoi(function_id_str);
       // printf("Function found with id: %d\n", function_id);
        
        // Return handle corresponding to function
        rpc_handle *handle = malloc(sizeof(rpc_handle));
        handle->function_id = function_id;

        return handle;
    }
    else {
       // printf("function not found\n");
        return NULL;
    }

    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    //printf("rpc_call called\n");
    char buffer[2000];
    char request[2000];
    int n, request_id;

    // Make sure inputs are not null
    if (cl == NULL || h == NULL || payload == NULL){
        return NULL;
    }
 
    // Send CALL request to server: request_id CALL function_id data1 data2_len data2 (using value from input rpc_data struct)
    request_id = cl->request_count;

    // Validate payload rpc_data
    if (validate_rpc_data(payload)){
        //printf("input rpc_data valid is valid\n");
        // Convert data2 byte array to string format
        //// printf("converting data2 to array string\n");
        //uint8_t data2_array[payload->data2_len];
        char data2_array_str[1000];
        strcpy(data2_array_str, "");
        char data2_elem[snprintf(NULL, 0, "%d,", 1) + 1];
        //strcpy(data2_array, *(uint8_t *)payload->data2);
        for (int i=0; i<payload->data2_len; i++){
            //printf("data2_array[%d]: %d\n", i, *(uint8_t *)(payload->data2+i));
            sprintf(data2_elem, "%d,", *(uint8_t *)(payload->data2+i));
            strcat(data2_array_str, data2_elem);
        }
        //("data2 array string: %s\n", data2_array_str);
        
        sprintf(request, "%d CALL %d %d %ld %s", request_id, h->function_id, payload->data1, payload->data2_len, data2_array_str);
    } else {
        //printf("invalid input rpc_data\n");
        return NULL;
    }

    //printf("sending request: %s\n", request);
    n = write(cl->sockfd, request, strlen(request));
	if (n < 0) {
		perror("socket");
		return NULL;
	}
    // printf("CALL request sent\n");

    // Get function result
    n = read(cl->sockfd, buffer, 255);
    buffer[n] = '\0';

    // Check that ID of response corresponds to ID of request
    int response_id = atoi(strtok(buffer, " "));
    //printf("response id: %d, request id: %d\n", response_id, request_id);
    while (response_id!=request_id && n>0){
        n = read(cl->sockfd, buffer, 255);
        buffer[n] = '\0';
        response_id = atoi(strtok(buffer, " "));
        //printf("Received function result: %s\n", buffer);
    }
    if (response_id!=request_id) {
        return NULL;
    }

    // Check if function was called successfully
    char *ok_message = strtok(NULL, " ");
    //printf("ok message: %s\n", ok_message);

    atoi(strtok(NULL, " ")); // skipping over function_id

    if (strcmp(ok_message, "OK")==0){
        int data1 = atoi(strtok(NULL, " "));
        //printf("data1: %d\n", data1);
        int data2_len = atoi(strtok(NULL, " "));
        //printf("data2_len: %d\n", data2_len);

        // Parse data2 byte array string into appropriate format to be used by function
        void *data2;
        if (data2_len>0){
            char data2_array_str[data2_len*2 + 1];
            //printf("blah\n");
            strcpy(data2_array_str, strtok(NULL, " "));
            //printf("blah\n");
            //printf("data2 array str: %s\n", data2_array_str);
            uint8_t *data2_array = malloc(sizeof(uint8_t) * (data2_len + 1));
            data2_array[0] = atoi(strtok(data2_array_str, ","));
            //printf("data2_array[0]: %d\n", data2_array[0]);
            for (int i=1; i<data2_len; i++){
                data2_array[i] = atoi(strtok(NULL, ","));
                //printf("data2_array[%d]: %d\n", i, data2_array[i]);
            }
            data2 = data2_array;
        }
        else {
            data2 = NULL;
        }
        
        // Return rpc_data struct containing result of function
        rpc_data *data = malloc(sizeof(rpc_data));
        data->data1 = data1;
        data->data2_len = data2_len;
        data->data2 = data2;

        return data;
    }
    else {
        //printf("error calling function\n");
        return NULL;
    }
}

void rpc_close_client(rpc_client *cl) {
   // printf("closing client\n");
    close(cl->sockfd);
   // printf("closed socket\n");
    free(cl);
   // printf("freed client\n");
}

void rpc_data_free(rpc_data *data) {
   // printf("freeing\n");
    if (data == NULL) {
        return;
    }
   // printf("freeing data2\n");
    if (data->data2 != NULL) {
        free(data->data2);
    }
   // printf("freed data2\n");
    free(data);
   // printf("freed data\n");
}
