#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "client.h"
#include "protocol.h"

int connect_to_server(void) {
  int client_fd;
  struct sockaddr_in server_address;

  client_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (client_fd < 0) {
    printf("socket");
    return -1;
  }

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);

  if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
    printf("inet_pton");
    close(client_fd);
    return -1;
  }

  if (connect(client_fd, (struct sockaddr *)&server_address,
              sizeof(server_address)) < 0) {

    printf("connect");
    close(client_fd);
    return -1;
  }

  printf("Connected to server.\n");
  return client_fd;
}

uint64_t get_file_size(char *filename) {
  struct stat file_info;

  if (stat(filename, &file_info) < 0) {
    printf("stat");
    return 0;
  }

  return (uint64_t)file_info.st_size;
}

int send_file(int client_fd, char *filename) {
  int file_fd = open(filename, O_RDONLY);

  if (file_fd < 0) {
    printf("open");
    return -1;
  }

  uint64_t file_size = get_file_size(filename);

  // send filename length
  uint32_t filename_length = 0;
  while (filename[filename_length] != '\0') {
    filename_length++;
  }

  uint32_t filename_length_network = htonl(filename_length);

  if (send_all(client_fd, &filename_length_network,
               sizeof(filename_length_network)) < 0) {
    printf("Error sending filename length.\n");
    close(file_fd);
    return -1;
  }

  // send filename
  if (send_all(client_fd, filename, filename_length) < 0) {
    printf("Error sending filename.\n");
    close(file_fd);
    return -1;
  }

  // send file size
  if (send_all(client_fd, &file_size, sizeof(file_size)) < 0) {
    printf("Error sending file size.\n");
    close(file_fd);
    return -1;
  }

  // send file
  char buffer[4096];
  ssize_t bytes_read;

  while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
    if (send_all(client_fd, buffer, bytes_read) < 0) {
      printf("Error sending file.\n");
      close(file_fd);
      return -1;
    }
  }

  if (bytes_read < 0) {
    printf("read");
    close(file_fd);
    return -1;
  }

  close(file_fd);
  printf("File sent successfully.\n");
  return 0;
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    exit(-1);
  }

  char *filename = argv[1];

  int client_fd = connect_to_server();
  if (client_fd < 0) {
    exit(-1);
  }

  if (send_file(client_fd, filename) < 0) {
    close(client_fd);
    exit(-1);
  }

  char buffer[8192];
  ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_received < 0) {
    printf("recv");
    close(client_fd);
    exit(-1);
  }

  buffer[bytes_received] = '\0';

  printf("\nServer Output:\n");
  printf("%s\n", buffer);

  close(client_fd);
  return 0;
}