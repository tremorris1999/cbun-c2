#include "command.h"
#include "socket_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int parse_params(command_t *command, char *src_buf, size_t offset,
                 size_t length) {
  char *buf = &src_buf[offset];
  int pairs_found = 1;
  for (size_t i = 0; i < length; i += 1) {
    // printf("%c\n", buf[i]);
    if (buf[i] == ';') {
      pairs_found += 1;
    }
  }

  // printf("found %d params\n", pairs_found);
  kvp_t **params = calloc(pairs_found, sizeof(kvp_t *));
  if (!params)
    return -1;

  for (size_t i = 0; i < pairs_found; i += 1) {
    params[i] = calloc(1, sizeof(kvp_t));
    if (!params[i])
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
      params[pair_count]->key =
          calloc(offsets.k_end - offsets.k_start + 1, sizeof(char));

      if (!params[pair_count]->key)
        return -1;

      memcpy(params[pair_count]->key, buf + offsets.k_start,
             offsets.k_end - offsets.k_start);
    } else if (buf[i] == ';' || i == length) {
      offsets.v_end = i;
      params[pair_count]->value =
          calloc(offsets.v_end - offsets.v_start + 1, sizeof(char));

      if (!params[pair_count]->value)
        return -1;

      memcpy(params[pair_count]->value, buf + offsets.v_start,
             offsets.v_end - offsets.v_start);
      pair_count += 1;
      offsets.k_start = i + 1;
    }

    i += 1;
  }

  command->params = params;
  command->params_length = pair_count;
  return pair_count;
}

command_t *recv_command(int sockfd) {
  if (sockfd < 0)
    return NULL;

  uint8_t len_buf[4];
  if (recv(sockfd, &len_buf, 4 * sizeof(uint8_t), 0) < 4) {
    fprintf(stderr, "invalid packet length.\n");
    return NULL;
  }

  uint32_t data_len = read_uint32(len_buf, 0);
  uint8_t bytes_recd = 0;
  uint8_t *data_buf = calloc(data_len + 1, sizeof(uint8_t));
  if (!data_buf) {
    fprintf(stderr, "failed to allocate\n");
    return NULL;
  }

  while (bytes_recd < data_len) {
    int cur_recd = recv(sockfd, data_buf, data_len, 0);
    switch (cur_recd) {
    case 0:
      fprintf(stderr, "socket closed.\n");
      return NULL;
    case -1:
      fprintf(stderr, "bad socket.\n");
      return NULL;
    default:
      bytes_recd += cur_recd;
      break;
    }
  }

  command_t *command = calloc(1, sizeof(command_t));
  if (!command)
    return NULL;

  memcpy(command->id, data_buf, 36);
  command->type = data_buf[36];

  uint32_t length = read_uint32(data_buf, 37);
  int params_len = parse_params(command, (char *)data_buf, 41, length);
  if (!params_len)
    command = NULL;

  free(data_buf);
  return command;
}

void free_command(command_t *command) {
  kvp_t **params = command->params;
  for (size_t i = 0; i < command->params_length; i += 1) {
    kvp_t *param = params[i];
    free(param->key);
    free(param->value);
    free(param);
  }

  free(params);
  free(command);
}
