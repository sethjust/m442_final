#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#define TCP_PROTOCOL 6

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
