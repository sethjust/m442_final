#ifndef OBJ_H
#define OBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MODULUS (1<<16) //FIXME

typedef unsigned int hash_t;

/* An object is identified by its name and a random identifier. */
typedef struct _obj_t {
    int salt;
    char *name;
    hash_t hash;
    char *bytes;
    char *metadata; /* job or file, output filename, completed, etc.
      Format is:
        For a file: "FILE\0"
        For a job: "JOB:outfilehash:complete\0"
          where complete is one of '0' or '1'
      
      */
} obj_t;

typedef struct _node_t {
    int salt;
    char *address;
    int port;
    hash_t hash;
    enum _type {LOCAL, REMOTE} type;
} node_t;

hash_t hash(obj_t* obj);
obj_t *Obj(int salt, char *name, char *bytes, char *metadata);
void free_obj(obj_t *obj);
char *tostr(obj_t* obj);

hash_t hash_node(node_t *node);
node_t *Node(int salt, char *address, int port);
void free_node(node_t *node);

#endif
