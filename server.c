// INCLUDES

//#include <sys/socket.h>
//#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// DEFINES

#define MODULUS 256 //FIXME

// TYPES

// An object is identified by its name and a random identifier.
// TODO: add data carrying
typedef struct _obj_t {
  int id;
  char* name;
} obj_t;

typedef struct _server_t {
  enum _type {LOCAL, REMOTE} type;
} server_t; //FIXME

// FUNCTIONS

obj_t* Obj(int id, char* name) {
  // Pseudo-constructor for objects.
  // Note that the name is copied, so the input may be reused.

  obj_t* obj = (obj_t*) malloc(sizeof(obj_t));
  obj->id = id;
  obj->name = (char*) malloc(strlen(name) * sizeof(char));
  strcpy(obj->name, name);

  return obj;
}

char* tostr(obj_t* obj){
  char* str = malloc(256 * sizeof(char));
  sprintf(str, "{ %d : %s }", obj->id, obj->name);
  return str;
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

int is_local(server_t* server) {
  return (server->type == LOCAL);
}

void local_add(obj_t* obj) { //FIXME
  printf("adding %s locally\n", tostr(obj));
}

void remote_add(server_t* server, obj_t* obj) { //FIXME
  printf("adding %s to remote server\n", tostr(obj));
}

void add(obj_t* obj) {
  long n = hash(obj);

  printf("key is %lx for %s\n", n, tostr(obj));

  server_t* server = next_server(n);

  if (is_local(server)) {
    local_add(obj);
  } else {
    remote_add(server, obj);
  }
}

int main(int argc, char** argv) {
  char str[256];

  int i = 0;
  while (1) {
    gets(str);

    obj_t* obj = Obj(i, str);
    add(obj);

    i++;
  }
  return 0;
}
