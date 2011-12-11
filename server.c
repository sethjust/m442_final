#include "server.h"

// FUNCTIONS

static void add_node_self_first(int count);
static void add_node_self(int count);
static void pop_and_gets(queue_t *queue);
static int process_gets_response(char *buffer);
static void send_sadds(void);

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
  char* buffer = message_node(node, addstr(obj));
  if (strcmp(buffer, "ACK")) return -1;
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

char* remote_get_bytes(node_t* node, hash_t n) {
  char* request = malloc(13*sizeof(char));

  sprintf(request, "GET:%08X", n);
  return message_node(node, request);
}

obj_t* get_object(hash_t n) {
  return local_get_object(n);
}

int remote_update_job(node_t* node, hash_t n, char* out, char* err) { //FIXME
  return 0;
}

int update_job(hash_t n, char* out, char* err) {
  node_t* node = next_node(n);

  if (is_local(node)) {
    char *save;
    obj_t* o = local_get_object(n);
    
//  For a job: "JOB:outputhash{:inputhash}*\0"
    if (strncmp(o->metadata, "JOB", 3)) return -1;

    char *key = strtok_r(o->metadata, ":", &save);
    key = strtok_r(NULL, ":", &save);

    int m;
    htoi(key, &m);

    local_update_bytes(n, err); /* save the stderr output to the job's bytes
                                   TODO: decide if we really want to overwrite
                                   the code. atm, this makes for easier gets
                                   local_update_bytes(m, out); save stdout to the
                                   outputfile */
    local_update_bytes(m, out);
    return 0;
  } else {
    return remote_update_job(node, n, out, err);
  }
}

bool file_is_ready(hash_t n) { 
  node_t* node = next_node(n);

  if (is_local(node)) {
    return local_file_is_ready(n);
  } else {
//    obj_t* obj = remote_get_object(node, n);
//    return obj->complete;
    return true; //FIXME
  }
}

int salt_counter = 0;

queue_t *my_queue = NULL;
char* process_msg(char *message) {
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
            }

            free_node(node);
        }

        return "ACK";
    }
    else if (!strcmp(head, "ADD")) {

        char *name = strtok_r(NULL, ":", &save_ptr);
        char *bytes = strtok_r(NULL, ":", &save_ptr);

        if (bytes == NULL) return "NACK";

        obj_t* obj = Obj(salt_counter++, name, bytes, "FILE", 1); // use & increment the salt

        if (add(obj)) return "NACK"; 

        char* buffer = malloc((22) * sizeof(char));
        sprintf(buffer, "ACK:%s", tostr(obj));
        return buffer;
    }
    else if (!strcmp(head, "BADD")) {
        char *name = strtok_r(NULL, ":", &save_ptr);
        char *salt = strtok_r(NULL, ":", &save_ptr);
        bool complete = *(strtok_r(NULL, ":", &save_ptr))=='0';
        char *bytes = strtok_r(NULL, ":", &save_ptr);

        int n;
        if  (8 != htoi(salt, &n)) return "NACK";

        if (bytes == NULL) return "NACK";

        obj_t* obj = Obj(n , name, bytes, "FILE", complete);

        if (local_add(obj)) return "NACK";

        free_obj(obj);
        return "ACK";
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

        buffer = strdup(save_ptr);

        int n;
        while ((input = strtok_r(NULL, ":", &save_ptr)) != NULL) {
          result = htoi(input, &n);
          if (!(result == 8)) {
            printf("did not parse entire hash\n");
            return "NACK";
          }
          if (!(file_is_ready(n))) { 
            printf("file was not ready\n");
            return "NACK";
          }
        }

        // create empty file
        int m = salt_counter++; // use & increment the salt
        obj_t* f = Obj(m, output, "", "FILE", 0);
        add(f);
        
//        For a job: "JOB:outputhash{:inputhash}*\0"
        metadata = (char*) malloc((14+strlen(buffer))*sizeof(char));
        sprintf(metadata, "JOB:%08X:%s", hash(output, m), buffer);


        obj_t* obj = Obj(salt_counter++, name, bytes, metadata, 0); // use & increment the salt //TODO: not atomic

        if (add(obj)) return "NACK";

        buffer = (char*)malloc(22*sizeof(char));
        sprintf(buffer, "ACK:%08X:%08X", obj->hash, hash(output, m));
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

        node_t* node = next_node(n);

        if (is_local(node)) {
          obj = get_object(n); /* FIXME: Should not return objects that are not
                                        marked as complete.*/

          buffer = (char*) malloc((5 + strlen(obj->bytes)) * sizeof(char));
          sprintf(buffer, "ACK:%s", obj->bytes);

          return buffer;
        } else {
          return remote_get_bytes(node, n);
        }
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
      char *buffer, *save, *outhash, *files, *hash;
      obj_t* obj = local_next_job(0); //FIXME: locality
      hash_t n;

      if (obj == NULL) return "NACK";
      
//        For a job: "JOB:outputhash{:inputhash}*\0"
      strtok_r(obj->metadata, ":", &save);
      outhash = strtok_r(NULL, ":", &save);

      files = (char*)malloc(sizeof(char));
      *files = 0;
      while ((hash = strtok_r(NULL, ":", &save)) != NULL) {
        if (!(htoi(hash, &n)==8)) {
            printf("did not parse entire hash\n");
            return "NACK";
        }
        obj_t* o = get_object(n);
        buffer = (char*)malloc((strlen(files)+10+strlen(o->name)+1)*sizeof(char));
        hash[8]=0;
        sprintf(buffer, "%s:%s:%s", files, hash, o->name);
        free(files);
        files = buffer;
      }

      buffer = (char*)malloc(sizeof(char)*(8+strlen(obj->bytes)+strlen(outhash)+strlen(files)));
      sprintf(buffer, "ACK:%s:%08X%s", obj->bytes, obj->hash, files);
      return buffer;
    }
    //TODO: SUCC
    else if (!strcmp(head, "UPD")) {
//    UPD:jobhash:stdout:stderr -> ACK -- update job status/results

      char *key = strtok_r(NULL, ":", &save_ptr);
      char *out = strtok_r(NULL, ":", &save_ptr);
      char *err = strtok_r(NULL, ":", &save_ptr);

      if (err == NULL) {
          printf("did not get enough tokens\n");
          return "NACK"; //naïve
      }

      int n;
      if (!(htoi(key, &n)==8)) {
          printf("did not parse entire hash\n");
          return "NACK";
      }

      if (update_job(n, out, err)) {
          printf("failed to update job\n");
          return "NACK"; 
      }
      return "ACK";
    }
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

char* message_node(node_t* node, char* msg)
{
    char *resp, *buffer;
    int res;
    int connection;

    printf("connecting to %s:%d\n", node->address, node->port);
    /* Open a connection to our given server. */
    res = make_connection_with(node->address, node->port, &connection);
    if (res<0) return;

    send_message(connection, msg);
    recv_message(connection, &resp);

    send_message(connection, "STOP");
    recv_message(connection, &buffer);

    if (!strcmp(buffer, "NACK")) printf("STOP got NACK!\n");

    close(connection);
    
    return resp;
}

static void pop_and_gets(queue_t *queue)
{
    char* buffer;

    node_t *node = pop_queue(queue);
    buffer = message_node(node, "GETS");
    printf("%s\n", buffer);
    process_gets_response(buffer);
}


int listener;//This will need to change for threading
// Handle Ctrl-C to close sockets
void  INThandler(int sig)
{
  signal(sig, SIG_IGN); // Ignore Ctrl-C for the moment
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

  int result, connection;

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
    add_node_self(3);
    send_sadds();
  } else {
    add_node_self_first(3);
  }

  while (1) {
    // The (currently unthreaded) main loop waits for a connection, and then processes a series of messages
    result = wait_for_connection(listener, &connection);
    if (result < 0) {
      printf("failed to get connection (%s)\n", strerror(errno));
      return result;
    }

    pthread_t *tid = (pthread_t*)malloc(sizeof(pthread_t));
//    pthread_attr_t a;
//    pthread_attr_init(&a);

//    conn_listen(connection);
//    pthread_create(&tid,&a,(void*(*)(void *))conn_listen,(void *)&connection);
    int *conn = (int*)malloc(sizeof(int));
    *conn = connection;
    pthread_create(tid,NULL,(void*(*)(void *))conn_listen,(void *)conn);

//    close(connection);
  }

  return 0;
}

static void add_node_self_first(int count)
{
    int i;
    for (i = 0; i < count; i++) {
        node_t *node = Node(i, my_ip, my_port);
        node->type = LOCAL;
        local_add_node(node);
        free_node(node);
    }
}

static void add_node_self(int count)
{
    int j;
    int i = 0;
    node_t *node = Node(0, my_ip, my_port);
    node->type = LOCAL;
    local_add_node(node);

    for (j = 1; j < count; j++) {
        while (true) {
            node = Node(++i, my_ip, my_port);
            node->type = LOCAL;

            hash_t hash1 = next_node_loop(node->hash);
            hash_t hash2 = next_node_loop(hash1);
            node_t *node1 = local_get_node(hash1);
            node_t *node2 = local_get_node(hash2);
            if (node1->type == LOCAL || node2->type == LOCAL) {
                free_node(node1);
                free_node(node2);
                free_node(node);
                continue;
            } else {
                free_node(node1);
                free_node(node2);
                local_add_node(node);
                free_node(node);
                break;
            }
        }
    }
}

static int process_gets_response(char *buffer)
{
    char *save_ptr;
    char *head = strtok_r(buffer, ":", &save_ptr);
    if (!strcmp(head, "ACK")) {
        int result;
        char *host, *port, *salt;

        while ((host = strtok_r(NULL, ":", &save_ptr)) != NULL) {
            port = strtok_r(NULL, ":", &save_ptr);
            salt = strtok_r(NULL, ":", &save_ptr);

            if (salt == NULL) return -1;  // if we get three tokens, we're
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
    } else {
        return -1;
    }
    return 0;
}

static void send_sadds(void)
{
    char *message = malloc(sizeof(*message) * 512);
    char *temp = malloc(sizeof(*message) * 30);
    message[0] = '\0';
    strcat(message, "SADD");

    hash_t hash = 0;
    hash_t next_hash;
    while ((next_hash = next_local_hash(hash)) != hash) {
        node_t *node = local_get_node(next_hash);
        temp[0] = '\0';
        sprintf(temp, ":%s:%d:%d", node->address, node->port, node->salt);
        strcat(message, temp);
        free_node(node);
        hash = next_hash;
    }
    free(temp);
}
