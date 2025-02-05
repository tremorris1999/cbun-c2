#include <stddef.h>
#include <stdint.h>

typedef struct kvp_t {
  char *key;
  char *value;
} kvp_t;

enum command_type {
  EXIT = 0,
  ISSUE_SESSION_ID,
  RESUME_SESSION = 2,
  SUSPEND_SESSION = 3,
  SYS_CONF = 4
};

typedef struct command_t {
  char id[37];
  uint8_t type;
  kvp_t **params;
  uint32_t params_length;
  uint8_t *payload;
  uint32_t payload_length;
} command_t;

static int parse_params(command_t *command, char *buf, size_t offset,
                        size_t length);

command_t *recv_command(int sockfd);
void free_command(command_t *command);
