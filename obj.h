#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum _obj_type_t {
  DATA,
  JOB
} obj_type_t;

// An object is identified by its name and a random identifier.
// TODO: add data carrying
typedef struct _obj_t {
  obj_type_t* type;
  char* name;
  int salt;
} obj_t;

obj_t* Obj(int salt, char* name);
char* tostr(obj_t* obj);
