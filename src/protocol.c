#include <errno.h>
#include <sys/socket.h>

#include "protocol.h"

ssize_t send_all(int socket_fd, void *buffer, size_t size_to_be_sent) {
  size_t total_bytes_sent = 0;

  while (total_bytes_sent < size_to_be_sent) {
    ssize_t actual_bytes_sent =
        send(socket_fd, (char *)buffer + total_bytes_sent,
             size_to_be_sent - total_bytes_sent, 0);
    if (actual_bytes_sent < 0) {
      if (errno == EINTR) { // Interrupted system call
        continue;
      }
      return -1;
    }
    total_bytes_sent += actual_bytes_sent;
  }
  return total_bytes_sent;
}

ssize_t recv_all(int socket_fd, void *buffer, size_t size_to_be_received) {
  size_t total_bytes_received = 0;

  while (total_bytes_received < size_to_be_received) {
    ssize_t actual_bytes_received =
        recv(socket_fd, (char *)buffer + total_bytes_received,
             size_to_be_received - total_bytes_received, 0);

    if (actual_bytes_received < 0) {
      if (errno == EINTR) { // Interrupted system call
        continue;
      }
      return -1;
    }

    if (actual_bytes_received == 0) {
      return 0;
    }

    total_bytes_received += actual_bytes_received;
  }
  return total_bytes_received;
}