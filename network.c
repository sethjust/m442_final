#include "network.h"

int build_address(char *hostname, int port, struct sockaddr_in *inet_address) {
  // from Jim Fix
  struct hostent *host_entries;
  host_entries = gethostbyname(hostname);
  if (host_entries == NULL || host_entries->h_addrtype != AF_INET || host_entries->h_length != 4) {
    return -1;
  } else {
    // success!
    inet_address->sin_family = AF_INET;
    inet_address->sin_addr.s_addr = *((uint32_t *)host_entries->h_addr_list[0]);
    inet_address->sin_port = port;
    return 0;
  }
}

int set_up_listener(int port, int *listener) {
  // adapted from Jim Fix
  int listen_socket;
  char hostname[128];
  struct sockaddr_in address;
  int result;

  // get a socket
  listen_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL); 
  if (listen_socket == -1) return listen_socket;

  if (gethostname(hostname,sizeof(hostname)) < 0) return -1;
  result = build_address(hostname,port,&address);
  if (result < 0) return result;
  
  // bind it to a port on this machine
  result = bind(listen_socket,(struct sockaddr *)&address,sizeof(address)); 
  if (result < 0) return result;
  
  // return the listener's socket descriptor
  *listener = listen_socket;
  return 0;
}

int wait_for_connection(int listen_socket, int *connection) {
  // from Jim Fix
  int socket;

  // listen for a client's "connect" request
  if (listen(listen_socket, 1) < 0) return -1;

  // accept it, get a dedicated socket for connection with this client
  socket = accept(listen_socket,NULL,NULL);
  if (socket < 0) return -1;

  // return that client connection's socket descriptor
  *connection = socket;
  return 0;
}

ssize_t recv_string(int socket, char *buffer, size_t maxlen) {
  // from Jim Fix
  ssize_t rc;
  char c;
  int pos;

  for (pos = 0; pos <= maxlen; pos++) {
    if ((rc = read(socket, &c, 1))==1) {
      buffer[pos] = c;
      if (c==0) break;
    } else if (rc==0) {
      break;
    } else {
      if (errno != EINTR ) continue;
      return -1;
    }
  }
  buffer[pos] = 0;
  return pos;
}

ssize_t send_string(int socket, char *buffer) {
  // from Jim Fix
  ssize_t     nwritten;
  int pos = 0;
  int len = strlen(buffer)+1;
  while (pos < len) {
    if ((nwritten = write(socket, &(buffer[pos]), len-pos)) <= 0) {
      if (errno == EINTR) {
        nwritten = 0;
      } else {
        return -1;
      }
    }
    pos += nwritten;
  }
  return pos;
}

int send_message(int bulletin_socket, char* message, char** response) {
  int result;
  char* buffer = (char*) malloc(sizeof(char)*256);

  result = send_string(bulletin_socket,message);
  if (!(result == strlen(message))) return -1;
  
  result = recv_string(bulletin_socket,buffer,255);
  if (result < 0) return -1;

  *response = buffer;
  return 0;
}

void conn_listen(int socket, char* process(char*)) {
  char buffer[256];
  int length;
  // repeatedly respond to lines sent by the client
  do {
    // receive a string from this client's connection socket
    length = recv_string(socket,buffer,255);
//    printf("Client says \"%s\".\n",buffer);

    send_string(socket, process(buffer)); // Note that process() may have side effects.
  } while (length >= 0 && strcmp(buffer,"STOP")); 
}
