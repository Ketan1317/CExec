#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8000
#define BUFFER_SIZE 4096 // common 4KB block size in OS

ssize_t send_all(int socket_fd, void *buffer, ssize_t size_to_be_sent) {
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

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <source_file.c>\n", argv[0]);
    exit(-1);
  }

  char *filepath = argv[1];
  // create socket
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (client_fd < 0) {
    printf("\nSocket Error\n");
    exit(-1);
  }

  printf("Socket created successfully\n");

  // Configure server address - "Which server do I want to connect to?""
  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  // Server is running on our own machine
  // inet_pton - It converts an IP address written as a human-readable string
  // into the binary format required by the socket API.
  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
    printf("Server Error");
    close(client_fd);
    exit(-1);
  }
  // inet_pton()
  //  |
  //  | write converted IP here
  //  ↓
  // server_addr.sin_addr

  // Connect my client socket to this server address
  if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {

    printf("Connection Error");
    close(client_fd);
    exit(-1);
  }

  printf("Connected to server\n");

  // OPEN file
  int file_fd = open(filepath, O_RDONLY);

  if (file_fd < 0) {
    printf("Server Error\n");
    close(client_fd);
    exit(-1);
  }

  printf("Opened file: %s\n", filepath);

  // extract filename
  char *filename = strrchr(filepath, '/');
  if (filename != NULL) {
    filename++;
  } else {
    filename = filepath;
  }

  struct stat file_info;
  if (fstat(file_fd, &file_info) < 0) {
    perror("Error in file stat\n");
    close(file_fd);
    close(client_fd);
    exit(-1);
  }

  uint64_t file_size = file_info.st_size;
  printf("File size: %llu bytes\n", (unsigned long long)file_size);

  uint32_t filename_length = strlen(filename);
  // convert length to network byte order
  uint32_t filename_length_network = htonl(filename_length);

  // send filename length;
  if (send_all(client_fd, &filename_length_network,
               sizeof(filename_length_network)) < 0) {
    printf("Error in sending filename length\n");
    close(file_fd);
    close(client_fd);
    exit(-1);
  }
  // send filename
  if (send_all(client_fd, filename, filename_length) < 0) {
    printf("Error in sending filename\n");
    close(file_fd);
    close(client_fd);
    exit(-1);
  }

  uint64_t file_size_network = file_size;
  // sending file size
  if (send_all(client_fd, &file_size_network, sizeof(file_size_network)) < 0) {
    printf("Error in sending file size\n");
    close(file_fd);
    close(client_fd);
    exit(-1);
  }

  printf("Sending file...\n");

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
    if (send_all(client_fd, buffer, bytes_read) < 0) {
      printf("Error in sending file\n");
      close(file_fd);
      close(client_fd);
      exit(-1);
    }
  }

  if (bytes_read < 0) {
    perror("read");
    close(file_fd);
    close(client_fd);
    exit(-1);
  }

  printf("Source code sent.\n");
  shutdown(client_fd, SHUT_WR);

  char response[BUFFER_SIZE];
  ssize_t response_bytes = recv(client_fd, response, sizeof(response) - 1, 0);

  if (response_bytes < 0) {
    perror("recv");
    close(file_fd);
    close(client_fd);
    exit(-1);
  }

  response[response_bytes] = '\0';

  printf("\nSERVER RESULT\n");
  printf("%s\n", response);

  close(file_fd);
  close(client_fd);

  return 0;
}