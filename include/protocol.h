#ifndef PROTOCOL_H // include/file gaurd - preventing multiple inclusion of header files, which causes redefinition errors
#define PROTOCOL_H // define the macro

#include <stddef.h> // size_t
#include <sys/types.h> // ssize_t

ssize_t send_all(
    int socket_fd,
    void *buffer,
    size_t size
);

ssize_t recv_all(
    int socket_fd,
    void *buffer,
    size_t size
);

#endif