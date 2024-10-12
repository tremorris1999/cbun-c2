#include "command.h"
#include "socket_utils.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

void handle_reconnect(command_t *command, int *sockfd, char *session_id);

int main(int argc, char **argv) {
  char session_id[37] = {'\0'};
  uint16_t interval = 0;
  float jitter = 0;

  int sockfd = connect_socket(8080);
  command_t *command = recv_command(sockfd);
  while (command && command->type != EXIT) {
    switch (command->type) {
    case ISSUE_SESSION_ID: {
      if (!session_id[0])
        memcpy(session_id, command->id, 36);

      uint8_t buf[42] = {'\0'};
      write_uint32(buf, 42, 0);
      memcpy(&buf[4], session_id, 36);
      memset(&buf[40], RESUME_SESSION, 1);
      uint32_t len = read_uint32(buf, 0);
      send_buf(sockfd, buf, 41);
      break;
    }
    case SUSPEND_SESSION: {
      int16_t port = strtol(command->params[0]->value, NULL, 10);
      int32_t reconnect_time = strtol(command->params[1]->value, NULL, 10);
      double delay = difftime(reconnect_time, time(NULL));
      printf("sleeping for %.f...\n", delay);
      struct timespec remaining, request = {delay, 0};
      nanosleep(&request, &remaining);
      sockfd = connect_socket(port);
    }
    default: {
      // printf("invalid command type");
      break;
    }
    }

    free_command(command);
    command = recv_command(sockfd);
  }
  // while (1) {
  //   disconnect_socket(&sockfd);
  //   float random = (2.0f * rand() / RAND_MAX) - 1.0f; // -1 < r < 1
  //   float delay_s = interval + (jitter * random);
  //   float delay_ns = fmod(delay_s, 1) * 1000000000;
  //   struct timespec remaining, delay = {floorf(delay_s), delay_ns};
  //   printf("sleeping {%.2lf, %.2lf}...\n", floorf(delay_s), delay_ns);
  //   nanosleep(&delay, &remaining);

  //   sockfd = connect_socket(8080, session_id);
  //   send_buf(sockfd, (uint8_t *)command->id, 37);
  // }

  return 0;
}

void handle_reconnect(command_t *command, int *sockfd, char *session_id) {
  printf("%s: %s\n", command->params[0]->key, command->params[0]->value);
  printf("%s: %s\n", command->params[1]->key, command->params[1]->value);
  int32_t reconnect_time = strtol(command->params[1]->value, NULL, 10);
  int16_t port = strtol(command->params[0]->value, NULL, 10);
  char packet[39] = "1;";
  memcpy(&packet[2], command->id, 37);
  send_buf(*sockfd, (uint8_t *)packet, 39);
  free_command(command);
  disconnect_socket(sockfd);

  double delay = difftime(reconnect_time, time(NULL));
  printf("sleeping for %.f...\n", delay);
  struct timespec remaining, request = {delay, 0};
  nanosleep(&request, &remaining);
  *sockfd = connect_socket(port);
}
