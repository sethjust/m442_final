//#include <sys/socket.h>
//#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULUS 256 //FIXME

typedef struct _obj_t {
  int id;
  char* name;
} obj_t;

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

void add(obj_t* obj) {
  printf("key is %lx\n", hash(obj));
}

int main(int argc, char** argv) {
  char str[256];

  int i = 0;
  while (1) {
    gets(str);

    obj_t* obj;
    obj->id = i;
    obj->name = (char*) malloc(strlen(str) * sizeof(char));
    strcpy(obj->name, str);
    add(obj);
  }
  return 0;
}
