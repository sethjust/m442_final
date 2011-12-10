#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
//#include <unistd.h>

#include "network.h"
#include "obj.h"
#include "sql.h"

#define GETS_SIZE (512)

int is_local(node_t* node);
node_t* next_node(hash_t n);
int local_add(obj_t* obj);
int remote_add(node_t* node, obj_t* obj);
int add(obj_t* obj);
obj_t* remote_get_object(node_t* node, hash_t n);
obj_t* get_object(hash_t n);
int remote_update_job(node_t* node, hash_t n, char* out, char* err);
int update_job(hash_t n, char* out, char* err);
bool file_is_ready(hash_t n);
int init_server_table(char* server, int port);
char* process_msg(char *message);

#endif
