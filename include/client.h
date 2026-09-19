#ifndef CLIENT_H
#define CLIENT_H

#include <stdint.h>

#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"

int connect_to_server(void);
uint64_t get_file_size(char *filename);
int send_file(int client_fd, char *filename);

#endif