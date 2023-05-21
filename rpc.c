#define _POSIX_C_SOURCE 200112L
#include <netdb.h> 
#include <strings.h> 
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include "rpc.h"

#define MAX_FUNCTION_NAME_LEN 10000
#define MAX_PROTOCOL_MESSAGE_LEN 100050

/* Struct representing a functiosn registered on the server, containing relevant information */
typedef struct registered_function {
    int id;
    char name[MAX_FUNCTION_NAME_LEN];
    rpc_handler handler;
} registered_function;

struct rpc_server {
    int sockfd;
    int num_functions;
    registered_function **functions;
};

/* Checks that fields of rpc_data are valid */
int validate_rpc_data(rpc_data *data){
    if (data==NULL){
        return 0;
    }
    else if (data->data2_len<0){
        return 0;
    }
    else if (data->data2_len>0 && data->data2==NULL){
        return 0;
    }
    else if (data->data2_len==0 && data->data2!=NULL){
        return 0;
    }
    return 1;
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
    char service[snprintf(NULL, 0, "%d", port) + 1];
    sprintf(service, "%d", port);
	s = getaddrinfo(NULL, service, &hints, &res);

	if (s != 0) {
		return -1;
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		return -1;
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		return -1;
	}

	// Bind address to the socket
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        return -1;
    }
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
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
    char service[snprintf(NULL, 0, "%d", port) + 1];
    sprintf(service, "%d", port);
	s = getaddrinfo(hostname, service, &hints, &servinfo);
	if (s != 0) {
		return -1;
	}

	// Connect to first valid result
	for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1){
            continue;
        }

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sockfd);
	}
	if (rp == NULL) {
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

    rpc_server *server = malloc(sizeof(rpc_server));
    server->sockfd = sockfd;
    server->num_functions = 0;
    server->functions = malloc(sizeof(registered_function));

    return server;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    // Check that inputs are not null
    if (name==NULL || srv==NULL || handler==NULL){
        return -1;
    }

    // Make sure name is a valid size
    if (strlen(name)>MAX_FUNCTION_NAME_LEN){
        return -1;
    }

    int function_id = srv->num_functions;

    // Check if function with the same name already exists, if so overwrite it with the new function
    for (int i=0; i<srv->num_functions; i++){
        if (srv->functions[i]!=NULL && strcmp(srv->functions[i]->name, name)==0){
            function_id = i;
            free(srv->functions[i]);    // free memory of overwritten function
            break;
        }
    }

    registered_function *function = malloc(sizeof(registered_function));
    function->id = function_id;
    strcpy(function->name, name); 
    function->handler = handler;

    srv->functions[function_id] = function;
    if (function_id==srv->num_functions){   // new function added, so increase count
        srv->num_functions++;
    }

    return function->id;
}

/* Socket-related code adapted from workshop 9 code */
void rpc_serve_all(rpc_server *srv) {
    int sockfd, newsockfd, n;
    char buffer[MAX_PROTOCOL_MESSAGE_LEN];
    sockfd = srv->sockfd;

    // Listen on socket for connections
    if (listen(sockfd, 5) < 0) {
        return;
    }

    // Accept connection
    struct sockaddr_storage client_addr;
	socklen_t client_addr_size;
    client_addr_size = sizeof client_addr;
	newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
	if (newsockfd < 0) {
		return;
	}

    while (1){
        n = read(newsockfd, buffer, MAX_PROTOCOL_MESSAGE_LEN);

        if (n==0){
             continue;
        }

		if (n < 0) {
			return;
		}
        buffer[n] = '\0';

        // Process request from client & send a response
        char response[MAX_PROTOCOL_MESSAGE_LEN];
        int request_id = atoi(strtok(buffer, " "));
        char *request = strtok(NULL, " ");

        // FIND -> find function on server
        if (strncmp("FIND", request, 4)==0){
            char *request_data = strtok(NULL, " ");

            int function_found = 0;
            // Iterate through registered functions on server, finding function with matching name
            for (int i=0; i<srv->num_functions; i++){
                if (srv->functions[i]!=NULL && strcmp(srv->functions[i]->name, request_data)==0){
                    // Get function id
                    sprintf(response, "%d OK %d", request_id, srv->functions[i]->id);
                    function_found = 1;
                    break;
                }
            }
            if (!function_found){
                sprintf(response, "%d ER", request_id);
            }
        }
        // CALL -> call function and return its result
        else if (strncmp("CALL", request, 4)==0){

            // Convert request data to rpc_data struct
            int function_id = atoi(strtok(NULL, " "));
            int data1 = atoi(strtok(NULL, " "));
            size_t data2_len = atoi(strtok(NULL, " "));

            // Parse data2 byte array string into appropriate format to be used by function
            char data2_array_str[data2_len*2 + 1];
            strcpy(data2_array_str, strtok(NULL, " "));
            uint8_t *data2_array = malloc(sizeof(uint8_t) * (data2_len + 1));
            data2_array[0] = atoi(strtok(data2_array_str, ","));
            for (int i=1; i<data2_len; i++){
                data2_array[i] = atoi(strtok(NULL, ","));
            }

            void *data2 = data2_array;

            rpc_data *input = malloc(sizeof(rpc_data));
            input->data1 = data1;
            input->data2_len = data2_len;
            input->data2 = data2;

            // Call function
            rpc_handler handler = srv->functions[function_id]->handler;
            rpc_data *result = handler(input);

            // Validate result rpc_data
            if (validate_rpc_data(result)) {
                // Convert data2 byte array to string format
                char result_data2_array_str[result->data2_len*2];
                strcpy(result_data2_array_str, "");
                char data2_elem[snprintf(NULL, 0, "%d,", 1) + 1];

                for (int i=0; i<result->data2_len; i++){
                    sprintf(data2_elem, "%d,", *(uint8_t *)(result->data2+i));
                    strcat(result_data2_array_str, data2_elem);
                }
                
                sprintf(response, "%d OK %d %d %ld %s", request_id, function_id, result->data1, result->data2_len, result_data2_array_str);

            } else {
                // Error, result rpc_data is invalid
                sprintf(response, "%d ER", request_id);
            }

            rpc_data_free(result);
            input->data2 = NULL;
            rpc_data_free(input);
        }

        // Send response to client
        n = write(newsockfd, response, strlen(response));
		if (n < 0) {
			return;
		}
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

    rpc_client *client = malloc(sizeof(rpc_client));
    client->sockfd = sockfd;
    client->request_count = 0;

    return client;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    int n, request_id;
    char request[MAX_PROTOCOL_MESSAGE_LEN];
    char buffer[MAX_PROTOCOL_MESSAGE_LEN];

    // Make sure inputs are not null
    if (name==NULL || cl == NULL){
        return NULL;
    }

    // Send FIND request to server: single message of form "request_id FIND name"
    request_id = cl->request_count;
    cl->request_count++;
    sprintf(request, "%d FIND %s", request_id, name);

    n = write(cl->sockfd, request, strlen(request));
	if (n < 0) {
		return NULL;
	}
    
    // Get response from server
    n = read(cl->sockfd, buffer, MAX_PROTOCOL_MESSAGE_LEN);
    buffer[n] = '\0';

    // Check that ID of response corresponds to ID of request
    int response_id = atoi(strtok(buffer, " "));
    if (response_id!=request_id) {
        return NULL;
    }

    // Check if function was found
    char *ok_message = strtok(NULL, " ");
    if (strcmp(ok_message, "OK")==0){
        // Get function id from response
        int function_id = atoi(strtok(NULL, " "));
        
        // Return handle corresponding to function
        rpc_handle *handle = malloc(sizeof(rpc_handle));
        handle->function_id = function_id;

        return handle;
    }
    else {
        // Error occured on the server
        return NULL;
    }

    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    char buffer[MAX_PROTOCOL_MESSAGE_LEN];
    char request[MAX_PROTOCOL_MESSAGE_LEN];
    int n, request_id;

    // Make sure inputs are not null
    if (cl == NULL || h == NULL || payload == NULL){
        return NULL;
    }
 
    // Send CALL request to server
    request_id = cl->request_count;

    // Validate payload rpc_data
    if (validate_rpc_data(payload)){
        // Convert data2 byte array to string format
        char data2_array_str[payload->data2_len*2+1];
        strcpy(data2_array_str, "");
        char data2_elem[snprintf(NULL, 0, "%d,", 1) + 1];
        for (int i=0; i<payload->data2_len; i++){
            sprintf(data2_elem, "%d,", *(uint8_t *)(payload->data2+i));
            strcat(data2_array_str, data2_elem);
        }
        
        sprintf(request, "%d CALL %d %d %ld %s", request_id, h->function_id, payload->data1, payload->data2_len, data2_array_str);
    } else {
        return NULL;
    }

    n = write(cl->sockfd, request, strlen(request));
	if (n < 0) {
		perror("socket");
		return NULL;
	}

    // Get function result
    n = read(cl->sockfd, buffer, MAX_PROTOCOL_MESSAGE_LEN);
    buffer[n] = '\0';

    // Check that ID of response corresponds to ID of request
    int response_id = atoi(strtok(buffer, " "));
    while (response_id!=request_id && n>0){
        n = read(cl->sockfd, buffer, MAX_PROTOCOL_MESSAGE_LEN);
        buffer[n] = '\0';
        response_id = atoi(strtok(buffer, " "));
    }
    if (response_id!=request_id) {
        return NULL;
    }

    // Check if function was called successfully
    char *ok_message = strtok(NULL, " ");

    atoi(strtok(NULL, " ")); // skipping over function_id

    if (strcmp(ok_message, "OK")==0){
        int data1 = atoi(strtok(NULL, " "));
        int data2_len = atoi(strtok(NULL, " "));

        // Parse data2 byte array string into appropriate format to be used by function
        void *data2;
        if (data2_len>0){
            char data2_array_str[data2_len*2 + 1];
            strcpy(data2_array_str, strtok(NULL, " "));
            uint8_t *data2_array = malloc(sizeof(uint8_t) * (data2_len + 1));
            data2_array[0] = atoi(strtok(data2_array_str, ","));
            for (int i=1; i<data2_len; i++){
                data2_array[i] = atoi(strtok(NULL, ","));
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
        return NULL;
    }
}

void rpc_close_client(rpc_client *cl) {
    close(cl->sockfd);
    free(cl);
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
