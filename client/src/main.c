#include "command.h"
#include "socket_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

void handle_reconnect(command_t *command, int *sockfd, char *session_id);

int main(int argc, char **argv) {
  char session_id[37] = {'\0'};
  int sockfd = connect_socket(8080, session_id);

  command_t *command = recv_command(sockfd);
  switch (command->type) {
  case EXIT:
    break;
  case SLEEP:
    handle_reconnect(command, &sockfd, session_id);
    break;
  }
  return 0;
}

void handle_reconnect(command_t *command, int *sockfd, char *session_id) {
  int32_t reconnect_time = strtol(command->params[0]->value, NULL, 10);
  int16_t port = strtol(command->params[1]->value, NULL, 10);
  free_command(command);

  char ack[38] = {'\0'};
  ack[0] = 1;
  memcpy(&ack[1], command->id, 37);
  send(*sockfd, ack, 38, 0);
  disconnect_socket(sockfd);

  double delay = difftime(reconnect_time, time(NULL));
  printf("sleeping for %.f...\n", delay);
  struct timespec remaining, request = {delay, 0};
  nanosleep(&request, &remaining);
  *sockfd = connect_socket(port, session_id);
}
