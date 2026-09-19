#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include <stdint.h>

#define STORAGE_DIR "storage"

#define BUFFER_SIZE 4096

int is_filename_safe(char *filename);

int receive_file(int client_fd, char *filename, uint64_t file_size);

int compile_program(char *filepath, char *executable);

int run_program(char *executable);

int read_file_output(char *buffer, size_t buffer_size);

#endif