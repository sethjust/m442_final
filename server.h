#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include "network.h"
#include "obj.h"
#include "sql.h"

#include <signal.h>

int is_local(node_t* node);
node_t* next_node(hash_t n);
int local_add(obj_t* obj);
int remote_add(node_t* node, obj_t* obj);
int add(obj_t* obj);
int init_server_table(char* server, int port);
char* process_msg(char *message);

#endif
