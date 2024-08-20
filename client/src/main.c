#include "command.h"
#include "socket_utils.h"

int main(int argc, char **argv) {
  char session_id[37] = {'\0'};
  int sockfd = connect_socket(8080, session_id);

  command_t *command = recv_command(sockfd);

  free_command(command);
  return 0;
}
