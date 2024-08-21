#include "command.h"
#include "socket_utils.h"
#include <math.h>
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

  int sockfd = connect_socket(8080, session_id);
  command_t *command = recv_command(sockfd);
  switch (command->type) {
  case EXIT:
    break;
  case CONFIG: {
    char packet[39] = "1;";
    memcpy(&packet[2], command->id, 37);
    send_buf(sockfd, (uint8_t *)packet, 39);
    interval = strtol(command->params[0]->value, NULL, 10);
    jitter = strtof(command->params[1]->value, NULL);
    free_command(command);
    break;
  }
  case SLEEP:
    handle_reconnect(command, &sockfd, session_id);
    break;
  }

  while (1) {
    disconnect_socket(&sockfd);
    float random = (2.0f * rand() / RAND_MAX) - 1.0f; // -1 < r < 1
    float delay_s = interval + (jitter * random);
    float delay_ns = fmod(delay_s, 1) * 1000000000;
    struct timespec remaining, delay = {floorf(delay_s), delay_ns};
    printf("sleeping {%.2lf, %.2lf}...\n", floorf(delay_s), delay_ns);
    nanosleep(&delay, &remaining);

    sockfd = connect_socket(8080, session_id);
    send_buf(sockfd, (uint8_t *)command->id, 37);
  }

  return 0;
}

void handle_reconnect(command_t *command, int *sockfd, char *session_id) {
  int32_t reconnect_time = strtol(command->params[0]->value, NULL, 10);
  int16_t port = strtol(command->params[1]->value, NULL, 10);
  char packet[39] = "1;";
  memcpy(&packet[2], command->id, 37);
  send_buf(*sockfd, (uint8_t *)packet, 39);
  free_command(command);
  disconnect_socket(sockfd);

  double delay = difftime(reconnect_time, time(NULL));
  printf("sleeping for %.f...\n", delay);
  struct timespec remaining, request = {delay, 0};
  nanosleep(&request, &remaining);
  *sockfd = connect_socket(port, session_id);
}
