#include "server.h"

// FUNCTIONS

static void add_node_self(int count);
static void pop_and_gets(queue_t *queue);

sqlite3 *db_file;
sqlite3 *db_node;

int is_local(node_t* node) {
  return (node->type == LOCAL);
}

node_t* next_node(hash_t n) {
    node_t *node;
    hash_t next_hash = next_node_hash(n);

    if (next_hash == n) {
        node = local_get_node(next_node_hash(0));
    } else {
        node = local_get_node(next_hash);
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
  hash_t n = hash(obj->name, obj->salt);

  node_t* node = next_node(n);

  if (is_local(node)) {
    return local_add(obj);
  } else {
    return remote_add(node, obj);
  }
}

int salt_counter = 0;

queue_t *my_queue = NULL;
char* process_msg(char* message) {
  printf("got message %s\n", message);

    char *save_ptr;
    char *head = strtok_r(message, ":", &save_ptr);

    if (!strcmp(head, "STOP")) {
        return "ACK";
    }
    else if (!strcmp(head, "SADD")) {
        int result;
        char *host, *port, *salt;

        while ((host = strtok_r(NULL, ":", &save_ptr)) != NULL) {
            port = strtok_r(NULL, ":", &save_ptr);
            salt = strtok_r(NULL, ":", &save_ptr);

            if (salt == NULL) return "NACK";  // if we get three tokens, we're
                                              // (naïvely) good
            htoi(salt, &result);
            node_t *node = Node(result, host, atoi(port));
            node->type = REMOTE;

            if (!node_hash_exists(node->hash)) {
                printf("adding %s:%d:%s\n", host, atoi(port), salt);
                local_add_node(node);
                if (my_queue != NULL) {
                    push_queue(my_queue, node);
                }
            }

            free_node(node);
        }

        return "ACK";
    }
    else if (!strcmp(head, "ADD")) {

        char *name = strtok_r(NULL, ":", &save_ptr);
        char *bytes = strtok_r(NULL, ":", &save_ptr);

        if (bytes == NULL) return "NACK";

        obj_t* obj = Obj(salt_counter++, name, bytes, "FILE:1"); // use & increment the salt

        if (local_add(obj)) return "NACK"; /* FIXME: Shouldn't always be local. */

        char* buffer = malloc((22) * sizeof(char));
        sprintf(buffer, "ACK:%s", tostr(obj));
//        if (file_hash_exists(obj->hash)) {
//            printf("%s\n", tostr(local_get_object(obj->hash)));
//        } else {
//            printf("could not find hash %08X\n", obj->hash);
//        }
        return buffer;
    }
    else if (!strcmp(head, "BADD")) {

        char *name = strtok_r(NULL, ":", &save_ptr);
        char *bytes = strtok_r(NULL, ":", &save_ptr);

        if (bytes == NULL) return "NACK";

        obj_t* obj = Obj(salt_counter++, name, bytes, "FILE:1"); // use & increment the salt

        if (local_add(obj)) return "NACK";

        char* buffer = malloc((22) * sizeof(char));
        sprintf(buffer, "ACK:%s", tostr(obj));
        free_obj(obj);
        return buffer;
    }
    else if (!strcmp(head, "JADD")) {
        int result;
        char *name, *bytes, *output, *input;
        char *metadata, *buffer;

        name = strtok_r(NULL, ":", &save_ptr);
        bytes = strtok_r(NULL, ":", &save_ptr);
        output = strtok_r(NULL, ":", &save_ptr);

        if (output == NULL) {
          return "NACK";
        }
        
//        For a job: "JOB:complete:outfilename:outfilesalt{:inputhash}*\0"
        metadata = (char*) malloc(16*sizeof(char));
        sprintf(metadata, "JOB:0:%s:00000000", output); //FIXME: create dummy file

        while ((input = strtok_r(NULL, ":", &save_ptr)) != NULL) {
          buffer = (char*)malloc((strlen(metadata)+10)*sizeof(char));
          sprintf(buffer, "%s:%s", metadata, input);
          metadata = buffer;
        }

        obj_t* obj = Obj(salt_counter++, name, bytes, metadata); // use & increment the salt //TODO: not atomic

        if (local_add(obj)) return "NACK"; /* FIXME: Shouldn't always be local. */

        buffer = (char*)malloc(13*sizeof(char));
        sprintf(buffer, "ACK:%08X", obj->hash);
        return buffer;
    }
    else if (!strcmp(head, "GET")) {

        char *key, *buffer;
        int n;
        obj_t *obj;

        key = strtok_r(NULL, ":", &save_ptr);

        if (key == NULL) {
            printf("did not get hash\n");
            return "NACK";
        }
        if (!(htoi(key, &n)==8)) {
            printf("did not parse entire hash\n");
            return "NACK";
        }

        obj = local_get_object(n); /* FIXME: Shouldn't always be local. */

        buffer = (char*) malloc((5 + strlen(obj->bytes)) * sizeof(char));
        sprintf(buffer, "ACK:%s", obj->bytes);

        return buffer;
    }
    else if (!strcmp(head, "GETS")) {
        printf("got gets\n");

        char *buffer = malloc(sizeof(*buffer) * GETS_SIZE);
        char *temp = malloc(sizeof(*temp) * 40);
        sprintf(buffer, "ACK");
        hash_t hash = 0;
        while (hash != next_node_hash(hash)) {
            hash = next_node_hash(hash);
            node_t *node = local_get_node(hash);
            sprintf(temp, ":%s:%d:%08X", node->address, node->port, node->salt);
            strcat(buffer, temp);
            free_node(node);
        }
        free(temp);
        return buffer;
    }
    else if (!strcmp(head, "GETJ")) {
//    GETJ -> ACK:sourcebytes:outputname:outputsalt{:inputhash}*
      char *buffer, *save, *outname, *outsalt, *files, *hash;
      hash_t n = 0; //FIXME: start in _local_ keyspace
      obj_t* obj;
      
      for (;;) { //FIXME: add a next job query?
        n = next_object_hash(n);
        obj = local_get_object(n);

        // Metadata for a job:
        // "JOB:complete:outfilename:outfilesalt{:inputhash}*\0"
        printf("%s\n", obj->metadata);
        buffer = strtok_r(obj->metadata, ":", &save);
        if (!strcmp(buffer, "JOB")) {
          buffer = strtok_r(NULL, ":", &save);
          if (!strcmp(buffer, "0")) {
            break;
          }
        }
      }
      outname = strtok_r(NULL, ":", &save);
      outsalt = strtok_r(NULL, ":", &save);

      files = (char*)malloc(sizeof(char));
      *files = 0;
      while ((hash = strtok_r(NULL, ":", &save)) != NULL) {
        buffer = (char*)malloc((strlen(files)+10)*sizeof(char));
        if (!(htoi(hash, &n)==8)) {
            printf("did not parse entire hash\n");
            return "NACK";
        }
        hash[8]=0;
        sprintf(buffer, "%s:%s:%s", files, hash, local_get_object(n)->name);
        files = buffer;
      }

      buffer = (char*)malloc(sizeof(char)*(8+strlen(obj->bytes)+strlen(outname)+strlen(outsalt)+strlen(files)));
      sprintf(buffer, "ACK:%s:%s:%s%s", obj->bytes, outname, outsalt, files);
      return buffer;
    }
    //TODO: SUCC
    else {
        return "NACK";
    }
}

int init_server_table(char* server, int port) {
    my_queue = new_queue();
    node_t *node = Node(0, server, port);
    push_queue(my_queue, node);
    while (!is_empty(my_queue)) {
        pop_and_gets(my_queue);
    }
    my_queue = NULL;
}

static void pop_and_gets(queue_t *queue)
{
    char* buffer;
    int res;
    int connection;

    node_t *node = pop_queue(queue);

    printf("connecting to %s:%d\n", node->address, node->port);
    /* Open a connection to our given server. */
    res = make_connection_with(node->address, node->port, &connection);
    if (res<0) return;

    send_message(connection, "GETS");
    recv_message(connection, &buffer);
    printf("%s\n", buffer);

    send_message(connection, "STOP");
    recv_message(connection, &buffer);
    printf("%s\n", buffer);

    close(connection);
}


int listener, connection;//This will need to change for threading
// Handle Ctrl-C to close sockets
void  INThandler(int sig)
{
  signal(sig, SIG_IGN); // Ignore Ctrl-C for the moment
  close(connection);
  close(listener);

  exit(0);
}

int my_port;
char *my_ip;

int main(int argc, char** argv) {
  signal(SIGINT, INThandler); // register a signal handler for Ctrl-C

  if (argc > 4) {
    fprintf(stderr,"server: too many arguments.\n");
    fprintf(stderr,"usage: server [localport] [server port] \n");
    fprintf(stderr,"\tStarts an object storage server on port, or 11111 if no port is given.\nContacts a given remote server to join a storage network.");
    exit(-1);
  }

  int result;

  if (argc < 2) {
    my_port = 11111;
  } else {
    my_port = atoi(argv[1]);
  }
  my_ip = get_self_ip();

  init_db();

  result = set_up_listener(my_port, &listener);
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
  add_node_self(3); /* FIXME: Test. */

  while (1) {
    // The (currently unthreaded) main loop waits for a connection, and then processes a series of messages
    result = wait_for_connection(listener, &connection);
    if (result < 0) {
      printf("failed to get connection (%s)\n", strerror(errno));
      return result;
    }

    conn_listen(connection);

    close(connection);
  }

  return 0;
}

static void add_node_self(int count)
{
    int i;
    for (i = 0; i < count; i++) {
        node_t *node = Node(i, my_ip, my_port);
        node->type = LOCAL;
        local_add_node(node);
        free_node(node);
    }
}
