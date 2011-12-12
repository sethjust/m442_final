#include "obj.h"

hash_t hash(char *text, int salt)
{
  // adapted from https://www.cse.yorku.ca/~oz/hash.html
  hash_t result = 5381; // 0b1010100000101
  char* i;

  result += salt;
  for (i = text; i++; ) {
    if ((int) *i == 0) break;
    result = result * 33 + ((int) *i);
  }

  return result;
}

obj_t* Obj(int salt, char* name, char* bytes, char* metadata, int complete)
{
  // Pseudo-constructor for objects.
  // Note that inputs are copied to malloced memory, so the input buffers may be reused

  obj_t* obj = (obj_t*)malloc(sizeof(obj_t));

  obj->salt = salt;

  obj->name = strdup(name);
  obj->metadata = strdup(metadata);
  obj->bytes = strdup(bytes);

  obj->hash = hash(obj->name, obj->salt);

  obj->complete = complete;

  return obj;
}

void free_obj(obj_t *obj)
{
    free(obj->name);
    free(obj->metadata);
    if (obj->bytes != NULL) {
        free(obj->bytes);
    }
    free(obj);
}

char* tostr(obj_t* obj) // representation of obj in response to ADD
{
  char* str = malloc((12+strlen(obj->name)) * sizeof(char));
      // the formatting should add at most 12 characters
  sprintf(str, "%08X", obj->hash);
  return str;
}

char *addstr(obj_t* obj) { // add command for other servers
//    BADD:name:salt:complete:bytes -> ACK -- copy a file to a non-primary server
  char* buffer = malloc(sizeof(char)*(20+strlen(obj->name)+strlen(obj->bytes)+strlen(obj->metadata)));
  sprintf(buffer, "BADD:%s:%08X:%1d:%s:%s", obj->name, obj->salt, obj->complete, obj->metadata, obj->bytes);
  return buffer;
}

node_t *Node(int salt, char *address, int port)
{
    node_t *node = malloc(sizeof(*node));

    node->salt = salt;
    node->port = port;
    node->address = strdup(address);

    node->hash = hash(node->address, node->salt + node->port);
    return node;
}

void free_node(node_t *node)
{
    free(node->address);
    free(node);
}

bool is_empty(queue_t *queue)
{
    return (queue->next == NULL);
}

queue_t *new_queue(void)
{
    queue_t *queue = malloc(sizeof(*queue));
    queue->next = NULL;
    queue->last = NULL;

    return queue;
}

node_t *pop_queue(queue_t *queue)
{
    queue_node_t *popped;

    if (is_empty(queue)) {
        return NULL;
    } else {
        popped = queue->next;
        queue->next = popped->next;
        if (queue->last == popped) {
            queue->last = NULL;
        }
    }
    node_t *node = popped->node;
    free(popped);
    return node;
}

void push_queue(queue_t *queue, node_t *node)
{
    node_t *node_dup = Node(node->salt, node->address, node->port);
    node_dup->type = node->type;

    queue_node_t *queue_node = malloc(sizeof(*queue_node));
    queue_node->node = node_dup;
    queue_node->next = NULL;

    if (is_empty(queue)) {
        queue->next = queue_node;
        queue->last = queue_node;
    } else {
        (queue->last)->next = queue_node;
        queue->last = queue_node;
    }
}
