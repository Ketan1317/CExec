#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "file.h"
#include "protocol.h"

int is_filename_safe(char *filename) {
  if (strstr(filename, "..") != NULL)
    return 0;

  if (strchr(filename, '/') != NULL)
    return 0;

  if (strchr(filename, '\\') != NULL)
    return 0;

  return 1;
}

int receive_file(int client_fd, char *filename, uint64_t file_size) {
  char filepath[512];
  snprintf(filepath, sizeof(filepath), "%s/%s", STORAGE_DIR, filename);

  int file_fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (file_fd < 0) {
    printf("Error in opening created file");
    close(client_fd);
    return -1;
  }

  printf("Created file: %s\n", filepath);

  // Receive file data
  char buffer[BUFFER_SIZE];
  uint64_t total_received = 0;

  while (total_received < file_size) {
    size_t remaining_bytes = file_size - total_received;
    size_t chunk_size =
        remaining_bytes < BUFFER_SIZE ? remaining_bytes : BUFFER_SIZE;

    ssize_t bytes_received = recv(client_fd, buffer, chunk_size, 0);

    if (bytes_received < 0) {
      printf("Error in receiving the file");
      close(file_fd);
      close(client_fd);
      return -1;
    }

    if (bytes_received == 0) {
      printf("Client disconnected before upload completed.\n");
      close(file_fd);
      close(client_fd);
      return -1;
    }

    // write partially
    ssize_t total_written = 0;

    while (total_written < bytes_received) {
      ssize_t bytes_written =
          write(file_fd,
                (char *)buffer + total_written,
                bytes_received - total_written);

      if (bytes_written < 0) {
        printf("Error in writing file content\n");
        close(file_fd);
        close(client_fd);
        return -1;
      }

      total_written += bytes_written;
    }

    total_received += bytes_received;
  }

  close(file_fd);

  printf("Received %llu bytes.\n",
         (unsigned long long)total_received);

  return 0;
}

int compile_program(char *filepath, char *executable) {
  char compile_command[2048];

  snprintf(compile_command,
           sizeof(compile_command),
           "gcc %s -o %s 2> storage/error.txt",
           filepath,
           executable);

  printf("Compiling...\n");
  int compile_result = system(compile_command);
  return compile_result;
}

int run_program(char *executable) {
  char run_command[1024];
  snprintf(run_command,
           sizeof(run_command),
           "%s > storage/output.txt 2>&1",
           executable);

  // 2>&1 means: stderr -> stdout
  // So both normal output and errors go into output.txt.

  int run_result = system(run_command);
  return run_result;
}

int read_file_output(char *buffer, size_t buffer_size) {
  int output_file_fd = open("storage/output.txt", O_RDONLY);

  if (output_file_fd < 0) {
    perror("open");
    return -1;
  }

  ssize_t output_bytes =
      read(output_file_fd, buffer, buffer_size - 1);

  if (output_bytes < 0) {
    perror("read");
    close(output_file_fd);
    return -1;
  }

  buffer[output_bytes] = '\0';
  close(output_file_fd);

  return output_bytes;
}