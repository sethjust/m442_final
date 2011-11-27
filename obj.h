// An object is identified by its name and a random identifier.
// TODO: add data carrying
typedef struct _obj_t {
  int id;
  char* name;
} obj_t;

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
