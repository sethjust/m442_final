#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// An object is identified by its name and a random identifier.
// TODO: add data carrying
typedef struct _obj_t {
  int id;
  char* name;
} obj_t;

obj_t* Obj(int id, char* name);
char* tostr(obj_t* obj);
