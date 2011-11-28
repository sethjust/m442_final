#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include "network.h"
#include "obj.h"

#define MODULUS 256 //FIXME

typedef struct _server_t {
  enum _type {LOCAL, REMOTE} type;
} server_t; //TODO

int is_local(server_t* server);
long hash(obj_t* obj);
server_t* next_server(long n);
int local_add(obj_t* obj);
int remote_add(server_t* server, obj_t* obj);
int add(obj_t* obj);
