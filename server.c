#include "server.h"

// FUNCTIONS

int is_local(server_t* server) {
  return (server->type == LOCAL);
}

unsigned long hash(obj_t* obj) {
  // adapted from https://www.cse.yorku.ca/~oz/hash.html
  unsigned long result = 5381; // 0b1010100000101
  char* i;

  result += obj->salt;
  for (i=obj->name; i++; ) {
    if ((int) *i == 0) break;
    result = result * 33 + ((int) *i);
  }

  return result % MODULUS;
}

server_t* next_server(unsigned long n) { //FIXME
//  "SELECT * from servers WHERE hash > n ORDERED BY hash LIMIT 1
  server_t loc;
  loc.type = LOCAL;

  server_t rem;
  rem.type = REMOTE;

  if (n < MODULUS/2) {
    return &loc;
  } else {
    return &rem;
  }
}

int local_add(obj_t* obj) { //FIXME
  printf("adding %s locally\n", tostr(obj));
  return 0;
}

int remote_add(server_t* server, obj_t* obj) { //FIXME
  printf("adding %s to remote server\n", tostr(obj));
  return 0;
}

int add(obj_t* obj) {
  unsigned long n = hash(obj);

  server_t* server = next_server(n);

  if (is_local(server)) {
    return local_add(obj);
  } else {
    return remote_add(server, obj);
  }
}

int salt_counter = 0;
char* process_msg(char* message) {

  printf("got message %s\n", message);
  if (
      !strncmp(message, "STOP", strlen("STOP"))
      ) { 
//    printf("Got stop\n");
    return "ACK";
  }
  else if (
      !strncmp(message, "ADDS", strlen("ADDS"))
      && message[strlen("ADDS")] == ':'
      ) {
//    printf("Got adds\n");

    char *host, *port, *id;

    host = strtok(&(message[strlen("ADDS")]), ":");
    port = strtok(NULL, ":");
    id = strtok(NULL, ":");

    if ( id == NULL ) return "NACK";  // if we get three tokens, we're
                                      //(naïvely) good //FIXME
    return "NACK"; //TODO
  }
  else if (
      !strncmp(message, "ADD", strlen("ADD"))
      && message[strlen("ADD")] == ':'
      ) { 
//    printf("Got add\n");

    char *name;

    name = strtok(&(message[strlen("ADD")]), ":");

    if (name == NULL) return "NACK";

    add(Obj(salt_counter++, name)); // use & increment the salt

    return "ACK";
  }
  else {
    return "NACK";
  }
}

int main(int argc, char** argv) {
  if (argc > 2) {
    fprintf(stderr,"server: too many arguments.\n");
    fprintf(stderr,"usage: server [port]\n");
    fprintf(stderr,"\tStarts an object storage server on port, or 11111 if no port is given.\n");
    exit(-1);
  }

  int port;
  if (argc>1) {
    port = atoi(argv[1]);
  } else {
    port = 11111;
  }
  
  int result;
  int listener;
  int connection;

  result = set_up_listener(port, &listener);
  if (result < 0) {
    printf("failed to set up listener (%s)\n", strerror(errno));
    return result;
  }

  while (1) {
    // The (currently unthreaded) main loop waits for a connection, and then processes a series of messages
    result = wait_for_connection(listener, &connection);
    if (result < 0) {
      printf("failed to get connection (%s)\n", strerror(errno));
      return result;
    }

    conn_listen(connection, process_msg);

    close(connection);
  }

  return 0;
}

//  char str[256];
//
//  int i = 0;
//  while (1) {
//    gets(str);
//
//    obj_t* obj = Obj(i, str);
//    add(obj);
//
//    i++;
//  }

