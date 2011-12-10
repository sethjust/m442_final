#ifndef NETWORK_H
#define NETWORK_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>

#include "server.h"

#define TCP_PROTOCOL 6

int build_address(char *hostname, int port, struct sockaddr_in *inet_address);
int set_up_listener(int port, int *listener);
int make_connection_with(char *hostname, int port, int *connection);
int wait_for_connection(int listen_socket, int *connection);
ssize_t send_string(int socket, char *buffer);
ssize_t recv_string(int socket, char *buffer, size_t maxlen);
int send_message(int socket, char* message);
int recv_message(int socket, char** buffer);
int htoi (const char *ptr, int *result);
int csum(const char* msg);
void conn_listen(int* socket);
char *get_self_ip(void);

#endif
