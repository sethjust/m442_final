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

obj_t* Obj(int salt, char* name, char* bytes, char* metadata) {
  // Pseudo-constructor for objects.
  // Note that inputs are copied to malloced memory, so the input buffers may be reused

  obj_t* obj = (obj_t*) malloc(sizeof(obj_t));

  obj->salt = salt;

  obj->name = strdup(name);
  obj->metadata = strdup(metadata);
  obj->bytes = strdup(bytes);

  obj->hash = hash(obj);

  return obj;
}

char* tostr(obj_t* obj){
  char* str = malloc((12+strlen(obj->name)) * sizeof(char));
      // the formatting should add at most 12 characters
  sprintf(str, "%08X:%08X", obj->salt, obj->hash);
  return str;
}

hash_t hash_node(node_t *node) {
    /* a variant of DJB2 */
    hash_t result = 5381; // 0b1010100000101
    char* i;

    result += node->salt + node->port;
    for (i = node->address; i++; ) {
        if ((int) *i == 0) break;
        result = result * 33 + ((int) *i);
    }

    return result % MODULUS;
}

node_t *Node(int salt, char *address, int port)
{
    node_t *node = malloc(sizeof(*node));

    node->salt = salt;
    node->port = port;
    node->address = strdup(address);

    node->hash = hash_node(node);
    return node;
}
