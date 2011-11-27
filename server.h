typedef struct _server_t {
  enum _type {LOCAL, REMOTE} type;
} server_t; //TODO

int is_local(server_t* server) {
  return (server->type == LOCAL);
}
