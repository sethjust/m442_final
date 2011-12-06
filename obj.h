#ifndef OBJ_H
#define OBJ_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef unsigned int hash_t;

/* An object is identified by its name and a random identifier. */
typedef struct _obj_t {
    int salt;
    char *name;
    hash_t hash;
    char *bytes;
    char *metadata; /* job or file, output filename, completed, etc.
      Format is:
        For a file: "FILE:complete\0"
          where complete is one of '0' or '1'
        For a job: "JOB:complete:outfilename:outfilesalt{:inputhash}*\0"
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

typedef struct _queue_node_t {
    node_t *node;
    struct _queue_node_t *next;
} queue_node_t;

typedef struct _queue_t {
    queue_node_t *next;
    queue_node_t *last;
} queue_t;

hash_t hash(char *text, int salt);

obj_t *Obj(int salt, char *name, char *bytes, char *metadata);
void free_obj(obj_t *obj);
char *tostr(obj_t* obj);

node_t *Node(int salt, char *address, int port);
void free_node(node_t *node);

bool is_empty(queue_t *queue);
queue_t *new_queue(void);
node_t *pop_queue(queue_t *queue);
void push_queue(queue_t *queue, node_t *node);

#endif
