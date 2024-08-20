#include <arpa/inet.h>

int connect_socket(int16_t port, char *session_id);
uint32_t read_uint32(uint8_t *buf, size_t offset);
uint16_t read_uint16(uint8_t *buf, size_t offset);
void disconnect_socket(int *sockfd);
