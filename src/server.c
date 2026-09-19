#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file.h"
#include "protocol.h"
#include "server.h"

void *handle_client(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);

  printf("\nClient connected. Thread started.\n");

  uint32_t filename_length_network;
  if (recv_all(client_fd, &filename_length_network,
               sizeof(filename_length_network)) < 0) {
    printf("Error receiving filename length.\n");
    close(client_fd);
    return NULL;
  }

  uint32_t filename_length = ntohl(filename_length_network);
  if (filename_length == 0 || filename_length >= 512) {
    printf("Invalid filename length.\n");
    close(client_fd);
    return NULL;
  }

  char filename[512];
  if (recv_all(client_fd, filename, filename_length) < 0) {
    printf("Error receiving filename.\n");
    close(client_fd);
    return NULL;
  }
  filename[filename_length] = '\0';

  printf("Filename: %s\n", filename);

  if (!is_filename_safe(filename)) {
    printf("Unsafe filename.\n");
    close(client_fd);
    return NULL;
  }

  uint64_t file_size;
  if (recv_all(client_fd, &file_size, sizeof(file_size)) < 0) {
    printf("Error receiving file size.\n");
    close(client_fd);
    return NULL;
  }

  printf("File size: %llu bytes\n", (unsigned long long)file_size);

  if (receive_file(client_fd, filename, file_size) < 0) {
    printf("File receiving failed.\n");
    close(client_fd);
    return NULL;
  }

  char filepath[512];
  snprintf(filepath, sizeof(filepath), "%s/%s", STORAGE_DIR, filename);

  char executable[512];
  snprintf(executable, sizeof(executable), "./storage/program");

  int compile_result = compile_program(filepath, executable);
  if (compile_result != 0) {
    printf("Compilation failed.\n");
    char error_buffer[8192];

    int error_file_fd = open("storage/error.txt", O_RDONLY);
    if (error_file_fd >= 0) {
      ssize_t error_bytes =
          read(error_file_fd, error_buffer, sizeof(error_buffer) - 1);

      if (error_bytes > 0) {
        error_buffer[error_bytes] = '\0';
        send_all(client_fd, error_buffer, error_bytes);
      }
      close(error_file_fd);
    }
    close(client_fd);
    printf("Client thread finished.\n");
    return NULL;
  }

  printf("Compilation successful.\n");

  int run_result = run_program(executable);
  if (run_result != 0) {
    printf("Program exited with an error.\n");
  }

  char output_buffer[8192];
  int output_bytes = read_file_output(output_buffer, sizeof(output_buffer));

  if (output_bytes < 0) {
    printf("Could not read program output.\n");
    close(client_fd);
    return NULL;
  }

  if (send_all(client_fd, output_buffer, output_bytes) < 0) {
    printf("Error sending output.\n");
    close(client_fd);
    return NULL;
  }

  printf("Output sent to client.\n");

  close(client_fd);

  printf("Client disconnected.\n");
  printf("Client thread finished.\n");

  return NULL;
}

int main() {
  // setup the socket server
  int server_fd;
  struct sockaddr_in server_address;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    printf("socket");
    exit(-1);
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    printf("setsockopt");
    close(server_fd);
    exit(-1);
  }

  if (mkdir(STORAGE_DIR, 0755) < 0 && errno != EEXIST) {
    printf("mkdir");
    close(server_fd);
    exit(-1);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0) {
    printf("bind");
    close(server_fd);
    exit(-1);
  }

  if (listen(server_fd, 10) < 0) {
    printf("listen");
    close(server_fd);
    exit(-1);
  }

  printf("Server listening on port %d...\n", PORT);

  while (1) {
    struct sockaddr_in client_address;
    socklen_t client_address_length = sizeof(client_address);

    int client_fd = accept(server_fd, (struct sockaddr *)&client_address,
                           &client_address_length);

    if (client_fd < 0) {
      if (errno == EINTR) {
        continue;
      }

      printf("accept");
      continue;
    }
    printf("\nNew client accepted.\n");

    int *client_fd_ptr = malloc(sizeof(int));
    if (client_fd_ptr == NULL) {
      printf("malloc");
      close(client_fd);
      continue;
    }

    *client_fd_ptr = client_fd;
    pthread_t client_thread;

    int thread_result =
        pthread_create(&client_thread, NULL, handle_client, client_fd_ptr);

    if (thread_result != 0) {
      printf("pthread_create");
      free(client_fd_ptr);
      close(client_fd);
      continue;
    }

    pthread_detach(client_thread);

    printf("Thread created for client.\n");
  }

  close(server_fd);

  return 0;
}