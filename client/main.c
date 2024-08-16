#include <bits/types/struct_iovec.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

enum action_type {
  EXIT = 0,
  GET_ENV = 1,
  RUN_COMMAND = 2,
  SLEEP = 3
};

typedef struct action_t {
  char id[37];
  uint8_t action_type;
  uint32_t payload_len;
  char *payload;
} action_t;


uint8_t* recv_data(int sockfd);
action_t* build_action(uint8_t* action_data);

int main(int argc, char **argv)
{
  struct sockaddr_in client_addr = {'\0'};
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  char *end;
  client_addr.sin_port = htons(8080);
  client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  client_addr.sin_family = AF_INET;
  int res = connect(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr));
  if (res != 0) {
    int err = errno;
    fprintf(stderr, "err: %s\n", strerror(err));
  }

  uint8_t zero = 0;
  send(sockfd, &zero, 1, 0);

  uint8_t *recd = recv_data(sockfd);
  if (!recd) return 1;

  action_t *action = build_action(recd);
  printf("id: %s\n", action->id);
  printf("type: %d\n", action->action_type);
  printf("payload length: %d\n", action->payload_len);
  printf("payload: %s\n", action->payload);

  switch (action->action_type) {
    case EXIT:
      break;
    case GET_ENV:
      break;
    case RUN_COMMAND:
      break;
    case SLEEP:
      break;
  }

  free(action->payload);
  free(action);
  shutdown(sockfd, SHUT_RDWR);
  close(sockfd);
  return 0;
}

uint8_t* recv_data(int sockfd) {
  if (sockfd < 0) return NULL;

  uint8_t part = 0;
  uint32_t data_len = 0;
  for (size_t i = 0; i < 4; i += 1) {
    if (recv(sockfd, &part, sizeof(uint8_t), 0) < 1) {
      fprintf(stderr, "invalid packet length.\n");
      return NULL;
    } else {
      if (i == 0) data_len = part;
      else data_len = (data_len << 8) + part;
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

action_t* build_action(uint8_t *action_data) { 
  action_t *action = calloc(1, sizeof(action_t));
  if (!action) return NULL;
   
  memcpy(action->id, action_data, 36);
  action->action_type = action_data[36];

  uint32_t len = action_data[37];
  len = (len << 8) + action_data[38];
  len = (len << 8) + action_data[39];
  len = (len << 8) + action_data[40];
  action->payload_len = len;
  
  action->payload = calloc(len + 1, sizeof(uint8_t));
  memcpy(action->payload, &action_data[41], len);
  
  free(action_data);
  return action;
}
