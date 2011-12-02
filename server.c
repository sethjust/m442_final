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
    return "ACK";
  }
  else if (
      !strncmp(message, "SADD", strlen("SADD"))
      && message[strlen("SADD")] == ':'
      ) {

    char *host, *port, *salt;

    host = strtok(&(message[strlen("ADDS")]), ":");
    port = strtok(NULL, ":");
    salt = strtok(NULL, ":");

    if ( salt == NULL ) return "NACK";  // if we get three tokens, we're
                                      //(naïvely) good //FIXME

    printf("adding %s:%d:%s\n", host, atoi(port), salt); //FIXME: placeholder for call

    return "ACK";
  }
  else if (
      !strncmp(message, "ADD", strlen("ADD"))
      && message[strlen("ADD")] == ':'
      ) { 

    char *name;

    name = strtok(&(message[strlen("ADD")]), ":");

    if (name == NULL) return "NACK";

    add(Obj(salt_counter++, name)); // use & increment the salt

    return "ACK";
  }
  // BADD
  // JADD
  // GET
  else if (
      !strncmp(message, "GETS", strlen("GETS"))
      ) {
    printf("got gets\n");

    // get results from DB

    char* buffer = "ACK:127.0.0.1:11111:0"; //FIXME

    return buffer;
  }
  // SUCC
  else {
    return "NACK";
  }
}

int init_server_table(char* server, int port) {
  int res;
  int connection;
  
  // open a connection to our given server
  printf("connecting to %s:%d\n", server, port);
  res = make_connection_with(server, port, &connection);
  if (res<0) return res;

  send_message(connection, "GETS");
  send_message(connection, "STOP");
  close(connection);
}

int main(int argc, char** argv) {
  if (argc > 4) {
    fprintf(stderr,"server: too many arguments.\n");
    fprintf(stderr,"usage: server <localport> <server> <port> \n");
    fprintf(stderr,"\tStarts an object storage server on port, or 11111 if no port is given.\n");
    exit(-1);
  }

  int port;
  int result;
  int listener;
  int connection;

  if (argc < 2) {
    port = 11111;
  } else {
    port = atoi(argv[1]);
  }

  result = set_up_listener(port, &listener);
  if (result < 0) {
    printf("failed to set up listener (%s)\n", strerror(errno));
    return result;
  }
  
  if (argc > 1) {
    // bring us up on the network
    char* server = argv[2];
    int sport = atoi(argv[3]);
    if (init_server_table(server, sport)) {
      printf("failed to init server table (%s)\n", strerror(errno));
      return -1;
    }
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
