#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket_utils.h"

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

void disconnect_socket(int *sockfd) {
  shutdown(*sockfd, SHUT_RDWR);
  close(*sockfd);
  *sockfd = -1;
}

int send_buf(int sockfd, uint8_t *buf, size_t length) {
  int bytes_sent = 0;
  while (bytes_sent < 39) {
    int cur = send(sockfd, &buf[bytes_sent], length - bytes_sent, 0);
    switch (cur) {
    case -1:
    case 0:
      return 0;
      break;
    default:
      bytes_sent += cur;
    }
  }

  return 1;
}
