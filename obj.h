#ifndef OBJ_H
#define OBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAX_NAME_SIZE 256
#define MAX_META_SIZE 512

#define MODULUS (1<<16) //FIXME

typedef uint8_t byte_t;
typedef unsigned int hash_t;

/* An object is identified by its name and a random identifier. */
typedef struct _obj_t {
    int salt;
    char *name;
    hash_t hash;
    byte_t *bytes;
    size_t size; /* length of byte array since it is not null-terminated. */
    char *metadata; /* job or file, output filename, completed, etc. */
} obj_t;

typedef struct _server_t {
    int salt;
    char *address;
    int port;
    hash_t hash;
    enum _type {LOCAL, REMOTE} type;
} server_t; // TODO


hash_t hash(obj_t* obj);
obj_t* Obj(int salt, char* name, byte_t* bytes, size_t size, char* metadata);
char* tostr(obj_t* obj);

#endif
