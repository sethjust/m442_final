#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>

#include "network.h"
#include "obj.h"
#include "server.h"

#define MODULUS 256 //FIXME

// FUNCTIONS

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

  if (argc < 1) {
    printf("Usage: ./server port\n  Sets up a storage node listening on the specified port\n");
    return -1;
  }
  
  int result;
  int listener;

  result = set_up_listener(atoi(argv[0]), &listener);
  if (result < 0) return result;

  while (1) {}

  return 0;
}
