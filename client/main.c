#include <arpa/inet.h>
#include <bits/types/struct_iovec.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

enum action_type { EXIT = 0, SLEEP = 1 };

typedef struct action_t {
  char id[37];
  uint8_t action_type;
  uint32_t payload_len;
  char *payload;
} action_t;

typedef struct kvp_t {
  char *key;
  char *value;
} kvp_t;

uint8_t *recv_command(int sockfd);
action_t *build_action(uint8_t *action_data);
int connect_socket(int16_t port, char *session_id);
void close_socket(int *sockfd);
uint32_t read_uint32(uint8_t *buf, size_t offset);
uint16_t read_uint16(uint8_t *buf, size_t offset);
int parse_params(kvp_t ***params, char *buf, int length);
void free_params(kvp_t **params, int length);

int main(int argc, char **argv) {
  char session_id[37] = {'\0'};
  int sockfd = connect_socket(8080, session_id);
  // struct timespec ts = {0, 5000};

  // nanosleep(&ts, 0);
  // close_socket(&sockfd);

  // sockfd = connect_socket(8081, session_id);

  uint8_t *recd = recv_command(sockfd);
  if (!recd)
    return 1;

  action_t *action = build_action(recd);
  // printf("id: %s\n", action->id);
  // printf("type: %d\n", action->action_type);
  // printf("payload length: %d\n", action->payload_len);
  // printf("payload: %s\n", action->payload);

  kvp_t **params;
  int params_length =
      parse_params(&params, action->payload, action->payload_len);

  if (params_length < 0)
    return 1;

  free_params(params, params_length);
  free(action->payload);
  free(action);
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
  return 0;
}

int connect_socket(int16_t port, char *session_id) {
  struct sockaddr_in client_addr = {'\0'};
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  char *end;
  client_addr.sin_port = htons(port);
  client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  client_addr.sin_family = AF_INET;
  int res =
      connect(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr));
  if (res != 0) {
    int err = errno;
    fprintf(stderr, "err: %s\n", strerror(err));
    return -1;
  }

  uint8_t zero = 0;
  if (strlen(session_id) < 36) {
    send(sockfd, &zero, 1, 0);
    if (recv(sockfd, session_id, 36, 0) != 36) {
      fprintf(stderr, "err: invalid session id\n");
      return -1;
    }
  } else
    send(sockfd, session_id, 36, 0);

  printf("connected to %d as %s\n", port, session_id);
  return sockfd;
}

void close_socket(int *sockfd) {
  shutdown(*sockfd, SHUT_RDWR);
  close(*sockfd);
  *sockfd = -1;
}

uint8_t *recv_command(int sockfd) {
  if (sockfd < 0)
    return NULL;

  uint8_t part = 0;
  uint32_t data_len = 0;
  for (size_t i = 0; i < 4; i += 1) {
    if (recv(sockfd, &part, sizeof(uint8_t), 0) < 1) {
      fprintf(stderr, "invalid packet length.\n");
      return NULL;
    } else {
      if (i == 0)
        data_len = part;
      else
        data_len = (data_len << 8) + part;
    }
  }

  uint8_t bytes_recd = 0;
  uint8_t *data_buf = calloc(data_len, sizeof(uint8_t));
  if (!data_buf) {
    fprintf(stderr, "failed to allocate\n");
    return NULL;
  }

  while (bytes_recd < data_len) {
    int cur_recd = recv(sockfd, data_buf, data_len, 0);
    switch (cur_recd) {
    case 0:
      fprintf(stderr, "socket closed.\n");
      return 0;
    case -1:
      fprintf(stderr, "bad socket.\n");
      return NULL;
    default:
      bytes_recd += cur_recd;
      break;
    }
  }

  return data_buf;
}

uint32_t read_uint32(uint8_t *buf, size_t offset) {
  uint32_t val = 0;
  for (size_t i = 0; i < 4; i += 1)
    val = (val << 8) + buf[offset + i];

  return val;
}

uint16_t read_uint16(uint8_t *buf, size_t offset) {
  uint16_t val = 0;
  for (size_t i = 0; i < 2; i += 1)
    val = (val << 8) + buf[offset + i];

  return val;
}

action_t *build_action(uint8_t *action_data) {
  action_t *action = calloc(1, sizeof(action_t));
  if (!action)
    return NULL;

  memcpy(action->id, action_data, 36);
  action->action_type = action_data[36];

  uint32_t len = read_uint32(action_data, 37);
  action->payload_len = len;
  action->payload = calloc(len + 1, sizeof(char));
  memcpy(action->payload, &action_data[41], len);

  free(action_data);
  return action;
}

int parse_params(kvp_t ***params, char *buf, int length) {
  int pairs_found = 1;
  for (size_t i = 0; i < length; i += 1)
    if (buf[i] == ';')
      pairs_found += 1;

  // printf("found %d params\n", pairs_found);
  kvp_t **pairs = calloc(pairs_found, sizeof(kvp_t *));
  if (!pairs)
    return -1;

  for (size_t i = 0; i < pairs_found; i += 1) {
    pairs[i] = calloc(1, sizeof(kvp_t));
    if (!pairs[i])
      return -1;
  }

  struct kvp_offsets {
    size_t k_start;
    size_t k_end;
    size_t v_start;
    size_t v_end;
  };

  struct kvp_offsets offsets = {0, 0, 0, 0};
  size_t i = 0, pair_count = 0;
  while (pair_count < pairs_found) {
    if (buf[i] == '=') {
      offsets.k_end = i;
      offsets.v_start = i + 1;
      pairs[pair_count]->key =
          calloc(offsets.k_end - offsets.k_start + 1, sizeof(char));

      if (!pairs[pair_count]->key)
        return -1;
      
      memcpy(pairs[pair_count]->key, buf + offsets.k_start,
             offsets.k_end - offsets.k_start);
    } else if (buf[i] == ';' || i == length) {
      offsets.v_end = i;
      pairs[pair_count]->value =
          calloc(offsets.v_end - offsets.v_start + 1, sizeof(char));

      if(!pairs[pair_count]->value)
        return -1;

      memcpy(pairs[pair_count]->value, buf + offsets.v_start,
             offsets.v_end - offsets.v_start);
      pair_count += 1;
      offsets.k_start = i + 1;
    }

    i += 1;
  }

  *params = pairs;
  return pair_count;
}

void free_params(kvp_t **params, int params_length) {
  for (size_t i = 0; i < params_length; i += 1) {
    free(params[i]->key);
    free(params[i]->value);
    free(params[i]);
  }

  free(params);
}
