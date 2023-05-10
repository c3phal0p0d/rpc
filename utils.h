#ifndef UTILS_H
#define UTILS_H

int setup_server_socket(int port, struct sockaddr_in* address);
int setup_client_socket(const int port, struct sockaddr_in* server_address);

#endif