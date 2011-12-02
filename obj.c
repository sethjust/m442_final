#include "obj.h"

hash_t hash(obj_t* obj) {
  // adapted from https://www.cse.yorku.ca/~oz/hash.html
  hash_t result = 5381; // 0b1010100000101
  char* i;

  result += obj->salt;
  for (i=obj->name; i++; ) {
    if ((int) *i == 0) break;
    result = result * 33 + ((int) *i);
  }

  return result % MODULUS;
}

obj_t* Obj(int salt, char* name, byte_t* bytes, size_t size, char* metadata) {
  // Pseudo-constructor for objects.
  // Note that inputs are copied to malloced memory, so the input buffers may be reused

  obj_t* obj = (obj_t*) malloc(sizeof(obj_t));

  obj->salt = salt;
  obj->size = size;

  obj->name = (char*) malloc(strlen(name) * sizeof(char));
  strcpy(obj->name, name);
  obj->metadata = (char*) malloc(strlen(metadata) * sizeof(char));
  strcpy(obj->metadata, metadata);
  obj->bytes = (byte_t*) malloc(obj->size * sizeof(byte_t));
  memcpy((void*) obj->bytes, bytes, obj->size);

  obj->hash = hash(obj);

  return obj;
}

char* tostr(obj_t* obj){
  char* str = malloc((12+strlen(obj->name)) * sizeof(char));
      // the formatting should add at most 12 characters
  sprintf(str, "{ %08X : %s }", obj->hash, obj->name);
  return str;
}
