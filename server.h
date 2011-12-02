#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include "network.h"
#include "obj.h"
#include "sql.h"

int is_local(server_t* server);
server_t* next_server(hash_t n);
int local_add(obj_t* obj);
int remote_add(server_t* server, obj_t* obj);
int add(obj_t* obj);
int init_server_table(char* server, int port);
