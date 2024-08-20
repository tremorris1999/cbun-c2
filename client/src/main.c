#include "command.h"
#include "socket_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  char session_id[37] = {'\0'};
  int sockfd = connect_socket(8080, session_id);

  command_t *command = recv_command(sockfd);
  int16_t port = strtol(command->params[1]->value, NULL, 10);  
  free_command(command);
  disconnect_socket(&sockfd);

  sockfd = connect_socket(port, session_id);

  return 0;
}
