#include <stddef.h>
#include <stdint.h>

typedef struct kvp_t {
  char *key;
  char *value;
} kvp_t;

enum command_type { EXIT = 0, SLEEP = 1 };

typedef struct command_t {
  char id[37];
  enum command_type type;
  kvp_t **params;
  int params_length;
} command_t;

static int parse_params(command_t *command, char *buf, size_t offset,
                        size_t length);
command_t *recv_command(int sockfd);
void free_command(command_t *command);
