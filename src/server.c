#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8000
#define BUFFER_SIZE 4096
#define STORAGE_DIR "storage"

ssize_t recv_all(int socket_fd, void *buffer, ssize_t size_to_be_received) {
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

int is_filename_safe(char *filename) {
  if (strstr(filename, "..") != NULL)
    return 0;
  if (strchr(filename, '/') != NULL)
    return 0;
  if (strchr(filename, '\\') != NULL)
    return 0;

  return 1;
}

int main(void) {
  // create a tcp socket
  // AF_INET - Use IPv4 addresses
  // SOCK_STREAM - Use a stream-oriented socket
  // 0 - OS Choose the appropriate protocol automatically.
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    printf("\nError in creating Socket\n");
    exit(-1);
  }

  printf("Socket created Successfully\n");

  // TCP connections can remain temporarily associated with the old port even
  // after ctrl + c, commonly due to states such as TIME_WAIT. SO_REUSEADDR
  // helps you restart the server without unnecessarily waiting for that old
  // state to expire.

  int opt = 1;                 // means enable/turn on the socket option
  if (setsockopt(server_fd,    // Which socket are we configuring
                 SOL_SOCKET,   // "The option I'm configuring is a general
                               // socket-level option."
                 SO_REUSEADDR, // Allow this server to reuse a local
                               // address/port in situations where the old
                               // connection is still in a temporary state.
                 &opt, sizeof(opt)) < 0) {
    printf("\nError in setsockopt\n");
    close(server_fd);
    exit(-1);
  }

  // Configure server address
  // This structure stores the server's network address information.
  // server_addr
  // ┌──────────────────────┐
  // │ Address family       │ → IPv4
  // │ IP address           │ → 0.0.0.0
  // │ Port                 │ → 8000
  // └──────────────────────┘
  struct sockaddr_in server_addr;
  // sockaddr_in              sockaddr_in6
  //    IPv4                       IPv6

  server_addr.sin_family = AF_INET;         // This address is an IPv4 address.
  server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available local
                                            // network interfaces/IP addresses
  server_addr.sin_port = htons(PORT);
  // to follow standardize on network byte order, which is big-endian.

  // Bind socket to IP + PORT
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {

    printf("\nError in Binding\n");
    close(server_fd);
    exit(-1);
  }

  printf("Server bound to port %d.\n", PORT);

  // Put server_fd into listening mode and allow up to 10 pending connections in
  // the backlog.
  if (listen(server_fd, 10) < 0) {
    printf("\nError in Listening\n");
    close(server_fd);
    exit(-1);
  }

  printf("Server listening on port %d\n", PORT);

  if (mkdir(STORAGE_DIR, 0755) < 0) {
    if (errno != EEXIST) {
      printf("Error in creating Directory");
      close(server_fd);
      exit(-1);
    }
  }

  printf("Storage directory ready.\n");

  // wait for client
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  // Take an incoming client connection and create a new socket specifically for
  // communicating with that client. BLOCKING CALL
  int client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

  // server_fd - listening socket
  // client_fd = communication socket

  if (client_fd < 0) {
    printf("\nError in Accepting Client\n");
    close(server_fd);
    exit(-1);
  }

  printf("Client connected!\n");

  uint32_t filename_length;
  if (recv_all(client_fd, &filename_length, sizeof(filename_length)) <= 0) {
    printf("Error in receiving filename length");
    close(server_fd);
    close(client_fd);
    exit(-1);
  }

  filename_length = ntohl(filename_length);

  if (filename_length == 0 || filename_length >= 256) {
    printf("Invalid filename length\n");
    close(client_fd);
    close(server_fd);
    exit(-1);
  }

  // Receive filename
  char filename[256];
  if (recv_all(client_fd, &filename, filename_length) <= 0) {
    printf("Error in receiving filename\n");
    close(client_fd);
    close(server_fd);
    exit(-1);
  }

  filename[filename_length] = '\0';

  if (!is_filename_safe(filename)) {
    printf("Unsafe filename!! Rejected\n");
    close(server_fd);
    close(client_fd);
    exit(-1);
  }

  printf("Receiving file: %s\n", filename);

  uint64_t file_size_network;
  if (recv_all(client_fd, &file_size_network, sizeof(file_size_network)) <= 0) {
    printf("Error in receiving file size\n");
    close(server_fd);
    close(client_fd);
    exit(-1);
  }

  // For this project, we'll use the 64-bit value directly.
  // Both client and server are running on the same architecture
  // during development.

  uint64_t file_size = file_size_network;
  printf("File size: %llu bytes\n", (unsigned long long)file_size);

  // create file
  char filepath[512];

  // Same as path.join(STORAGE_DIR, filename) and store in filepath;
  snprintf(filepath, sizeof(filepath), "%s/%s", STORAGE_DIR, filename);

  int file_fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (file_fd < 0) {
    printf("Error in opening created file");
    close(server_fd);
    close(client_fd);
    exit(-1);
  }

  printf("Created file: %s\n", filepath);

  // Receive file data
  char buffer[BUFFER_SIZE];
  uint64_t total_received = 0;

  while (total_received < file_size) {
    ssize_t remaining_bytes = file_size - total_received;
    ssize_t chunk_size =
        remaining_bytes < BUFFER_SIZE ? remaining_bytes : BUFFER_SIZE;
    ssize_t bytes_received = recv(client_fd, buffer, chunk_size, 0);

    if (bytes_received < 0) {
      printf("Error in receiving the file");
      close(server_fd);
      close(client_fd);
      exit(-1);
    }
    if (bytes_received == 0) {
      printf("Client disconnected before upload completed.\n");
      close(file_fd);
      close(client_fd);
      close(server_fd);
      exit(-1);
    }

    // write parially
    ssize_t total_written = 0;
    while (total_written < bytes_received) {
      ssize_t bytes_written = write(file_fd, (char *)buffer + total_written,
                                    bytes_received - total_written);
      if (bytes_written < 0) {
        printf("Error in writing file content\n");
        close(file_fd);
        close(client_fd);
        close(server_fd);
        exit(-1);
      }

      total_written += bytes_written;
    }
    total_received += bytes_received;
  }

  close(file_fd);
  printf("Source code received.\n");

  // compile source code
  char executable[512];
  snprintf(executable, sizeof(executable), "submissions/program");

  char compile_command[2048];
  snprintf(compile_command, sizeof(compile_command),
           "gcc %s -o %s 2> submissions/error.txt", filepath, executable);

  printf("Compiling...\n");

  int compile_result = system(compile_command);

  if (compile_result != 0) {
    printf("Compilation failed. \n");

    int error_file_fd = open("submissions/error.txt", O_RDONLY);
    if (error_file_fd < 0) {
      printf("Error in Opening a file");
      close(server_fd);
      close(client_fd);
      exit(-1);
    } else {
      char error_buffer[BUFFER_SIZE];
      ssize_t error_bytes;
      error_bytes = read(error_file_fd, error_buffer, sizeof(error_buffer) - 1);

      if (error_bytes < 0) {
        perror("read");
        close(server_fd);
        close(client_fd);
        close(error_file_fd);
        exit(-1);
      }

      error_buffer[error_bytes] = '\0';

      char response[BUFFER_SIZE];
      snprintf(response, sizeof(response), "COMPILATION ERROR\n\n%s",
               error_buffer);

      send(client_fd, response, strlen(response), 0);
      close(error_file_fd);
    }

    close(client_fd);
    close(server_fd);

    return 0;
  }

  printf("Compilation successful.\n");

  printf("Running program...\n");

  char run_command[1024];
  snprintf(run_command, sizeof(run_command), "%s > submissions/output.txt 2>&1",
           executable);

  // 2>&1 means: stderr -> stdout
  // So both normal output and errors go into output.txt.

  int run_result = system(run_command);
  int output_file_fd = open("submissions/output.txt", O_RDONLY);

  if (output_file_fd < 0) {
    perror("fopen");
    close(client_fd);
    close(server_fd);
    return 0;
  }

  char output[BUFFER_SIZE];
  ssize_t output_bytes = read(output_file_fd, output, sizeof(output) - 1);
  output[output_bytes] = '\0';

  close(output_file_fd);

  char response[8192];

  if (run_result == 0) {
    snprintf(response, sizeof(response),
             "COMPILED SUCCESSFULLY\n\nPROGRAM OUTPUT:\n%s", output);
  } else {
    snprintf(response, sizeof(response),
             "PROGRAM TERMINATED WITH ERROR\n\nOUTPUT:\n%s", output);
  }

  send(client_fd, response, strlen(response), 0);

  printf("Result sent to client.\n");

  close(client_fd);
  close(server_fd);

  return 0;
}
