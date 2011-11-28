#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define TCP_PROTOCOL 6

int build_address(char *hostname, int port, struct sockaddr_in *inet_address);
int set_up_listener(int port, int *listener);
int wait_for_connection(int listen_socket, int *connection);
ssize_t recv_string(int socket, char *buffer, size_t maxlen);
ssize_t send_string(int socket, char *buffer);
int send_message(int bulletin_socket, char* message, char** response);
void conn_listen(int bulletin_socket, char* process(char*, char*), char* saddr);
