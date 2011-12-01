#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TCP_PROTOCOL 6

int build_address(char *hostname, int port, struct sockaddr_in *inet_address);
int set_up_listener(int port, int *listener);
int wait_for_connection(int listen_socket, int *connection);
ssize_t recv_string(int socket, char *buffer, size_t maxlen);
ssize_t send_string(int socket, char *buffer);
int send_message(int socket, char* message);
int htoi (const char *ptr, int *result);
int csum(const char* msg);
void conn_listen(int socket, char* process(char*));
