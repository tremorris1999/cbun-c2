#include "command.h"
#include "socket_utils.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

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
      send_buf(sockfd, buf, 41);
      break;
    }
    case SUSPEND_SESSION: {
      int16_t port = strtol(command->params[0]->value, NULL, 10);
      int32_t reconnect_time = strtol(command->params[1]->value, NULL, 10);
      double delay = difftime(reconnect_time, time(NULL));
      printf("sleeping for %.f...\n", delay);
      struct timespec remaining, request = {delay, 0};
      close(sockfd);
      nanosleep(&request, &remaining);
      sockfd = connect_socket(port);
      break;
    }
    case SYS_CONF: {
      uint32_t pages = sysconf(_SC_PHYS_PAGES);
      uint32_t page_size = sysconf(_SC_PAGE_SIZE);
      uint64_t mem = pages * page_size;
      printf("returning 'res=%ld'", mem);
      uint8_t buf[54] = {'\0'};
      write_uint32(buf, 54, 0);
      memcpy(&buf[4], session_id, 36);
      buf[40] = SYS_CONF;
      memcpy(&buf[41], "res=", 4);
      write_uint64(buf, mem, 45);
      send_buf(sockfd, buf, 53);
      break;
    }
    default: {
      printf("\ninvalid command type");
      break;
    }
    }

    free_command(command);
    command = recv_command(sockfd);
  }

  printf("\nEXIT received from server. Shutting down gracefully");
  free_command(command);
  return 0;
}
