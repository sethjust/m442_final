#include "obj.h"

obj_t* Obj(int salt, char* name) {
  // Pseudo-constructor for objects.
  // Note that the name is copied, so the input may be reused.

  obj_t* obj = (obj_t*) malloc(sizeof(obj_t));
  obj->salt = salt;
  obj->name = (char*) malloc(strlen(name) * sizeof(char));
  strcpy(obj->name, name);

  return obj;
}

char* tostr(obj_t* obj){
  char* str = malloc((12+strlen(obj->name)) * sizeof(char));
      // the formatting should add at most 12 characters
  sprintf(str, "{ %d : %s }", obj->salt, obj->name);
  return str;
}
