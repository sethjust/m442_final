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
//  char hostname[128];
  struct sockaddr_in address;
  int result;

  // get a socket
  listen_socket = socket(PF_INET, SOCK_STREAM, TCP_PROTOCOL); 
  if (listen_socket == -1) return listen_socket;

//  if (gethostname(hostname,sizeof(hostname)) < 0) return -1;
//  result = build_address(hostname,port,&address);
//  if (result < 0) return result;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; // bind to all interfaces
  address.sin_port = htons(port); // convert to network byte order, or else 11111 binds to 26411
  
  // bind it to a port on this machine
  result = bind(listen_socket,(struct sockaddr *)&address,sizeof(address)); 
  if (result < 0) return result;

  printf("Set up a listenter on port %d\n", port);
  
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

int htoi (const char *ptr, int *result) {
  // adapted from http://johnsantic.com/comp/htoi.html
  int value = 0;
  int count = 0;
  char ch = *ptr;

  for (;;) {
    if (ch >= '0' && ch <= '9')
      value = (value << 4) + (ch - '0');
    else if (ch >= 'A' && ch <= 'F')
      value = (value << 4) + (ch - 'A' + 10);
    else if (ch >= 'a' && ch <= 'f')
      value = (value << 4) + (ch - 'a' + 10);
    else {
      *result = value;
      return count;
    }
    ch = *(++ptr);
    count++;
  }
}

int csum(const char* msg) {
  int res;
  char i = *msg;
  for (;;) {
    if (i==0) return res;
    res = (res + i) % 256;
    i=*(++msg);
  }
}

void conn_listen(int socket, char* process(char*)) {
  char* buffer;
  char size[6]; // we allow 4 hex bytes for size; this allows messages to be at
                // most 65536 bytes in length
  int res, length, sum;
  // repeatedly respond to lines sent by the client
  do {
    // receive a string from this client's connection socket
    res = recv_string(socket,size,5);
    if (res!=6) {
      printf("only recv'd %d hex digits\n", res);
      send_string(socket, "NACK");
      continue;
    }

    printf("Header is \"%s\".\n",size);

    res = htoi(size, &sum); // parse the hex digits
    if (!(res==6)) {
      printf("only parsed %d hex digits\n", res);
      send_string(socket, "NACK");
      continue;
    }

    // now, noting that a hex digit is 4 bits, we can use bit ops instead of
    // string ones:
    length = sum >> 8; // drop the two characters off the right hand side
    sum = sum - (length << 8); // leave only the last two in sum

    buffer = (char*) malloc(length*sizeof(char));

    res = recv_string(socket, buffer, length);
    if (!(length == res)) {
      printf("message had length %d, but said it was %d\n", res, length);
      send_string(socket, "NACK");
      continue;
    }
    if (!((res = csum(buffer)) == sum)) {
      printf("\"%s\" had checksum %x, but said it was %x\n", buffer, res, sum);
      send_string(socket, "NACK");
      continue;
    }

    printf("Client says \"%s\".\n",buffer);

    send_string(socket, process(buffer)); // Note that process() may have side effects.
  } while (length > 0 && strcmp(buffer,"STOP")); 
}
