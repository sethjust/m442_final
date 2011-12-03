#include "server.h"

// FUNCTIONS

sqlite3 *db_file;
sqlite3 *db_node;

int is_local(node_t* node) {
  return (node->type == LOCAL);
}

node_t* next_node(hash_t n) { //FIXME
//  "SELECT * from servers WHERE hash > n ORDER BY hash LIMIT 1
  node_t* node = (node_t*) malloc(sizeof(node_t));

  if (n < (MODULUS/2)) {
    node->type = LOCAL;
  } else {
    node->type = REMOTE;
  }
  return node;
}

int local_add(obj_t* obj) {
  printf("adding %s locally\n", tostr(obj));
  return local_add_object(obj);
}

int remote_add(node_t* node, obj_t* obj) { //FIXME
  printf("adding %s to remote server\n", tostr(obj));
  return 0;
}

int add(obj_t* obj) {
  hash_t n = hash(obj);

  node_t* node = next_node(n);

  if (is_local(node)) {
    return local_add(obj);
  } else {
    return remote_add(node, obj);
  }
}

int salt_counter = 0;
char* process_msg(char* message) {
//  printf("got message %s\n", message);

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
                                      //(naïvely) good

    printf("adding %s:%d:%s\n", host, atoi(port), salt); //TODO: placeholder for call to db

    return "ACK";
  }
  else if (
      !strncmp(message, "ADD", strlen("ADD"))
      && message[strlen("ADD")] == ':'
      ) { 

    char *name;

    name = strtok(&(message[strlen("ADD")]), ":");

    if (name == NULL) return "NACK";

//    obj_t* Obj(int salt, char* name, byte_t* bytes, size_t size, char* metadata) {
    char *bytes = malloc(sizeof(*bytes));
    bytes[0]=0;
    obj_t* obj = Obj(salt_counter++, name, bytes, ""); // use & increment the salt //TODO: parse the full message

    if (add(obj)) return "NACK";

    char* buffer = (char*) malloc(20*sizeof(char));
    sprintf(buffer, "ACK:%08X", obj->hash);
    if (file_hash_exists(obj->hash)) {
        printf("%s\n", tostr(local_get_object(obj->hash)));
    } else {
        printf("could not find hash %08X\n", obj->hash);
    }
//    bool file_hash_exists(hash_t hash);
//    obj_t *local_get_object(hash_t hash)

    return buffer;
  }
  //TODO: BADD
  //TODO: JADD
  //TODO: GET
  else if (
      !strncmp(message, "GETS", strlen("GETS"))
      ) {
    printf("got gets\n");

    //TODO: get results from DB

    return "ACK:127.0.0.1:11111:0";
  }
  //TODO: SUCC
  else {
    return "NACK";
  }
}

int init_server_table(char* server, int port) {
  char* buffer;
  int res;
  int connection;
  
  // open a connection to our given server
  printf("connecting to %s:%d\n", server, port);
  res = make_connection_with(server, port, &connection);
  if (res<0) return res;

  send_message(connection, "GETS");
  recv_message(connection, &buffer);
  printf("%s\n", buffer);

  send_message(connection, "STOP");
  recv_message(connection, &buffer);
  printf("%s\n", buffer);

  close(connection);
}

int main(int argc, char** argv) {
  if (argc > 4) {
    fprintf(stderr,"server: too many arguments.\n");
    fprintf(stderr,"usage: server [localport] [server port] \n");
    fprintf(stderr,"\tStarts an object storage server on port, or 11111 if no port is given.\nContacts a given remote server to join a storage network.");
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

  init_db();

  result = set_up_listener(port, &listener);
  if (result < 0) {
    printf("failed to set up listener (%s)\n", strerror(errno));
    return result;
  }
  
  if (argc > 2) {
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
