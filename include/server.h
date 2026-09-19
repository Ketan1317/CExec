#ifndef SERVER_H
#define SERVER_H
#include <pthread.h>

#define PORT 8080
void *handle_client(void *arg);

#endif