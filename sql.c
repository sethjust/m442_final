#include "sql.h"

extern sqlite3 *db_file;
extern sqlite3 *db_node;

static void file_db_crash(void);
static void node_db_crash(void);
static void make_file_table(void);
static void make_node_table(void);

bool file_hash_exists(hash_t hash)
{
    sqlite3_stmt *p_stmn;
    int retv;
    bool exists;

    sqlite3_prepare_v2(db_file, "SELECT hash FROM files WHERE hash = ?", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);

    retv = sqlite3_step(p_stmn);
    if (retv == SQLITE_ROW) {
        exists = true;
    } else if (retv == SQLITE_DONE) {
        exists = false;
    } else {
        file_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return exists;
}

obj_t *local_get_object(hash_t hash)
{
    sqlite3_stmt *p_stmn;

    /* Create new object. */
    obj_t *obj = malloc(sizeof(*obj));

    /* Set hash. */
    obj->hash = hash;

    sqlite3_prepare_v2(db_file, "SELECT * FROM files WHERE hash = ?", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);

    if (sqlite3_step(p_stmn) == SQLITE_ROW) {
        /* Get name. */
        char *result = (char *) sqlite3_column_text(p_stmn, 1);
        obj->name = strdup(result);
        /* Get salt. */
        obj->salt = sqlite3_column_int(p_stmn, 2);
        /* Get metadata. */
        result = (char *) sqlite3_column_text(p_stmn, 3);
        obj->metadata = strdup(result);
        /* Get Base64 encoded version of byte content. */
        result = (char *) sqlite3_column_text(p_stmn, 4);
        obj->bytes = strdup(result);
    } else {
        file_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return obj;
}

hash_t next_object_hash(hash_t hash)
{
    sqlite3_stmt *p_stmn;
    hash_t next_hash;
    int retv;

    sqlite3_prepare_v2(db_file, "SELECT hash FROM files WHERE (hash > ?) ORDER BY hash ASC", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);

    retv = sqlite3_step(p_stmn);
    if (retv == SQLITE_ROW) {
        next_hash = sqlite3_column_int64(p_stmn, 0);
    } else if (retv == SQLITE_DONE) {
        next_hash = hash;
    } else {
        node_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return next_hash;
}

int local_add_object(obj_t *obj)
{
    sqlite3_stmt *p_stmn;

    sqlite3_prepare_v2(db_file, "INSERT INTO files VALUES (?, ?, ?, ?, ?)", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, obj->hash);
    sqlite3_bind_text(p_stmn, 2, obj->name, -1, NULL);
    sqlite3_bind_int(p_stmn, 3, obj->salt);
    sqlite3_bind_text(p_stmn, 4, obj->metadata, -1, NULL);
    sqlite3_bind_text(p_stmn, 5, obj->bytes, -1, NULL);

    if (sqlite3_step(p_stmn) != SQLITE_DONE) {
        file_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return 0;
}

int local_remove_object(hash_t hash)
{
    sqlite3_stmt *p_stmn;

    sqlite3_prepare_v2(db_file, "DELETE FROM files WHERE hash = ?", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);
    if (sqlite3_step(p_stmn) != SQLITE_DONE) {
        file_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return 0;
}

static void make_file_table(void)
{
    char *sql_create_files = "CREATE TABLE files (hash INTEGER PRIMARY KEY, name TEXT, salt INTEGER, metadata TEXT, bytes text)";
    if (sqlite3_exec(db_file, sql_create_files, NULL, NULL, NULL) != SQLITE_OK) {
        file_db_crash();
    }
}

bool node_hash_exists(hash_t hash)
{
    sqlite3_stmt *p_stmn;
    int retv;
    bool exists;

    sqlite3_prepare_v2(db_node, "SELECT hash FROM nodes WHERE hash = ?", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);

    retv = sqlite3_step(p_stmn);
    if (retv == SQLITE_ROW) {
        exists = true;
    } else if (retv == SQLITE_DONE) {
        exists = false;
    } else {
        node_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return exists;
}

int local_add_node(node_t *node)
{
    sqlite3_stmt *p_stmn;

    sqlite3_prepare_v2(db_node, "INSERT INTO nodes VALUES (?, ?, ?, ?, ?)", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, node->hash);
    sqlite3_bind_text(p_stmn, 2, node->address, -1, NULL);
    sqlite3_bind_int(p_stmn, 3, node->port);
    sqlite3_bind_int(p_stmn, 4, node->salt);
    sqlite3_bind_int(p_stmn, 5, node->type);

    if (sqlite3_step(p_stmn) != SQLITE_DONE) {
        node_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return 0;
}

int local_remove_node(hash_t hash)
{
    sqlite3_stmt *p_stmn;

    sqlite3_prepare_v2(db_node, "DELETE FROM nodes WHERE hash = ?", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);
    if (sqlite3_step(p_stmn) != SQLITE_DONE) {
        file_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return 0;
}

node_t *local_get_node(hash_t hash)
{
    sqlite3_stmt *p_stmn;

    node_t *node = malloc(sizeof(*node));

    /* Set hash. */
    node->hash = hash;

    sqlite3_prepare_v2(db_node, "SELECT * FROM nodes WHERE hash = ?", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);

    if (sqlite3_step(p_stmn) == SQLITE_ROW) {
        /* Get address. */
        char *result = (char *) sqlite3_column_text(p_stmn, 1);
        node->address = strdup(result);
        /* Get port number. */
        node->port = sqlite3_column_int(p_stmn, 2);
        /* Get salt. */
        node->salt = sqlite3_column_int(p_stmn, 3);
        /* Get type. */
        node->type = sqlite3_column_int(p_stmn, 4);
    } else {
        node_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return node;
}

hash_t next_node_hash(hash_t hash)
{
    sqlite3_stmt *p_stmn;
    hash_t next_hash;
    int retv;

    sqlite3_prepare_v2(db_node, "SELECT hash FROM nodes WHERE (hash > ?) ORDER BY hash ASC", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);

    retv = sqlite3_step(p_stmn);
    if (retv == SQLITE_ROW) {
        next_hash = sqlite3_column_int64(p_stmn, 0);
    } else if (retv == SQLITE_DONE) {
        next_hash = hash;
    } else {
        node_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return next_hash;
}

static void make_node_table(void)
{
    char *sql_create_nodes = "CREATE TABLE nodes (hash INTEGER PRIMARY KEY, address TEXT, port INTEGER, salt INTEGER, type INTEGER)";
    if (sqlite3_exec(db_node, sql_create_nodes, NULL, NULL, NULL) != SQLITE_OK) {
        file_db_crash();
    }
}

static void file_db_crash(void)
{
    fprintf(stderr, "SQLite: %s\n", sqlite3_errmsg(db_file));
    fprintf(stderr, "Critical database error. Shutting down...\n");
    sqlite3_close(db_node);
    sqlite3_close(db_file);
    exit(EXIT_FAILURE);
}

static void node_db_crash(void)
{
    fprintf(stderr, "SQLite: %s\n", sqlite3_errmsg(db_node));
    fprintf(stderr, "Critical database error. Shutting down...\n");
    sqlite3_close(db_file);
    sqlite3_close(db_node);
    exit(EXIT_FAILURE);
}

void init_db(void)
{
    /* Open a db file for writing values. */
    if (sqlite3_open("", &db_file) != SQLITE_OK)
    {
        fprintf(stderr, "SQLite: %s\n", sqlite3_errmsg(db_file));
        fprintf(stderr, "Critical database error. Shutting down...\n");
        sqlite3_close(db_file);
        exit(EXIT_FAILURE);
    }
    make_file_table();

    /* Open a db in memory for writing addresses. */
    if (sqlite3_open(":memory:", &db_node) != SQLITE_OK)
    {
        fprintf(stderr, "SQLite: %s\n", sqlite3_errmsg(db_node));
        fprintf(stderr, "Critical database error. Shutting down...\n");
        sqlite3_close(db_file);
        sqlite3_close(db_node);
        exit(EXIT_FAILURE);
    }
    make_node_table();
}
