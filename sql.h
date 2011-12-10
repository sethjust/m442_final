#ifndef SQL_H
#define SQL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <sqlite3.h>

#include "obj.h"

void init_db(void);

/* SQL object functions */
bool file_hash_exists(hash_t hash);
bool local_file_is_ready(hash_t hash);
obj_t *local_get_object(hash_t hash);
int local_add_object(obj_t *obj);
int local_update_bytes(hash_t hash, char* bytes);
int local_update_metadata(hash_t hash, char* bytes);
int local_remove_object(hash_t hash);
hash_t next_object_hash(hash_t hash);
obj_t *local_next_job(hash_t hash);

/* SQL node functions */
bool node_hash_exists(hash_t hash);
node_t *local_get_node(hash_t hash);
int local_add_node(node_t *node);
int local_remove_node(hash_t hash);
hash_t next_node_hash(hash_t hash);

#endif
