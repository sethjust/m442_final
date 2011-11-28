#include "server.h"

// FUNCTIONS

int is_local(server_t* server) {
  return (server->type == LOCAL);
}

long hash(obj_t* obj) { //FIXME
  long result;
  char ptr[256];

  sprintf(ptr, "%s%d", obj->name, obj->id);

  char* i;
  for (i=obj->name; i++; ) {
    if ((int) *i == 0) break;
    result = (result + (int) *i) % MODULUS;
  }
  return result;
}

server_t* next_server(long n) { //FIXME
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
  long n = hash(obj);

  printf("key is %lx for %s\n", n, tostr(obj));

  server_t* server = next_server(n);

  if (is_local(server)) {
    return local_add(obj);
  } else {
    return remote_add(server, obj);
  }
}

char* process_msg(char* message) {
  printf("got message %s", message);
  if (
      !strncmp(message, "STOP", 4)
      ) { 
    printf("Got stop\n");
    return "ACK";
  }
  if (
      !strncmp(message, "ADD", 3)
      && message[3]==':'
      ) { 
    printf("Got add\n");

    int i = 3;
    char *name, *id;

    name = strtok(&(message[i]), ":");
    id = strtok(NULL, ":");

    add(Obj(atoi(id), name));

    return "ACK";
  }
  return "NACK";
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

