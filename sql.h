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
obj_t *local_get_object(hash_t hash);
int local_add_object(obj_t *obj);
int local_remove_object(hash_t hash);

/* SQL node functions */
bool node_hash_exists(hash_t hash);
int local_add_node(node_t *node);
int local_remove_node(hash_t hash);

#endif
