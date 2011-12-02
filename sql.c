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
        const unsigned char *text = sqlite3_column_text(p_stmn, 1);
        obj->name = malloc(sizeof(*obj->name) * MAX_NAME_SIZE);
        strcpy(obj->name, text);
        /* Get salt. */
        obj->salt = sqlite3_column_int(p_stmn, 2);
        /* Get metadata. */
        text = sqlite3_column_text(p_stmn, 3);
        obj->metadata = malloc(sizeof(*obj->name) * MAX_META_SIZE);
        strcpy(obj->metadata, text);
        /* Get byte content and size. */
        obj->size = sqlite3_column_bytes(p_stmn, 4);
        obj->bytes = malloc(sizeof(*obj->bytes) * obj->size);
        const void *byte_ptr = sqlite3_column_blob(p_stmn, 4);
        memcpy((void *) obj->bytes, byte_ptr, obj->size);
    } else {
        file_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return obj;
}

int local_add_object(obj_t *obj)
{
    sqlite3_stmt *p_stmn;

    sqlite3_prepare_v2(db_file, "INSERT INTO files VALUES (?, ?, ?, ?, ?)", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, obj->hash);
    sqlite3_bind_text(p_stmn, 2, obj->name, -1, NULL);
    sqlite3_bind_int(p_stmn, 3, obj->salt);
    sqlite3_bind_text(p_stmn, 4, obj->metadata, -1, NULL);
    sqlite3_bind_blob(p_stmn, 5, (void *) obj->bytes, obj->size, NULL);

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
    char *sql_create_files = "CREATE TABLE files (hash INTEGER PRIMARY KEY, name TEXT, salt INTEGER, metadata TEXT, bytes BLOB)";
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

int local_add_node(server_t *server)
{
    sqlite3_stmt *p_stmn;

    sqlite3_prepare_v2(db_node, "INSERT INTO nodes VALUES (?, ?, ?, ?, ?)", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, server->hash);
    sqlite3_bind_text(p_stmn, 2, server->address, -1, NULL);
    sqlite3_bind_int(p_stmn, 3, server->port);
    sqlite3_bind_int(p_stmn, 4, server->salt);
    sqlite3_bind_int(p_stmn, 5, server->type);

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

server_t *local_get_server(hash_t hash)
{
    sqlite3_stmt *p_stmn;

    server_t *server = malloc(sizeof(*server));

    /* Set hash. */
    server->hash = hash;

    sqlite3_prepare_v2(db_node, "SELECT * FROM nodes WHERE hash = ?", -1, &p_stmn, NULL);
    sqlite3_bind_int64(p_stmn, 1, hash);

    if (sqlite3_step(p_stmn) == SQLITE_ROW) {
        /* Get address. */
        const unsigned char *text = sqlite3_column_text(p_stmn, 1);
        server->address = malloc(sizeof(*server->address) * strlen(text) + 1);
        strcpy(server->address, text);
        /* Get port number. */
        server->port = sqlite3_column_int(p_stmn, 2);
        /* Get salt. */
        server->salt = sqlite3_column_int(p_stmn, 3);
        /* Get type. */
        server->type = sqlite3_column_int(p_stmn, 4);
    } else {
        node_db_crash();
    }

    sqlite3_finalize(p_stmn);
    return server;
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
