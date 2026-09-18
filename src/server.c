#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 8000
#define BUFFER_SIZE 4096
#define FILENAME_SIZE 4096

int main(void){
    // create a tcp socket
    // AF_INET - Use IPv4 addresses
    // SOCK_STREAM - Use a stream-oriented socket
    // 0 - OS Choose the appropriate protocol automatically.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd < 0){
        printf("\nError in creating Socket\n");
        exit(-1); 
    }

    printf("Socket created Successfully\n");

    // TCP connections can remain temporarily associated with the old port even after ctrl + c, commonly due to states such as TIME_WAIT.
    // SO_REUSEADDR helps you restart the server without unnecessarily waiting for that old state to expire.

    int opt = 1; // means enable/turn on the socket option
    if (setsockopt(
            server_fd, // Which socket are we configuring
            SOL_SOCKET, // "The option I'm configuring is a general socket-level option."
            SO_REUSEADDR, // Allow this server to reuse a local address/port in situations where the old connection is still in a temporary state.
            &opt,
            sizeof(opt)) < 0)
    {
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

    server_addr.sin_family = AF_INET; // This address is an IPv4 address.
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all available local network interfaces/IP addresses
    server_addr.sin_port = htons(PORT);
    // to follow standardize on network byte order, which is big-endian.

    // Bind socket to IP + PORT
    if(bind(server_fd,
           (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0){

            printf("\nError in Binding\n");
            close(server_fd);
            exit(-1);

    }

    printf("Server bound to port %d.\n", PORT);

    // Put server_fd into listening mode and allow up to 10 pending connections in the backlog.
    if(listen(server_fd,10) < 0){
        printf("\nError in Listening\n");
        close(server_fd);
        exit(-1);
    }

    printf("Server listening on port %d\n", PORT);

    if (mkdir("submissions", 0755) < 0){
        // Ignore error if directory already exists
        printf("Using existing submissions directory.\n");
    }

    // wait for client 
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Take an incoming client connection and create a new socket specifically for communicating with that client.
    // BLOCKING CALL
    int client_fd = accept(server_fd, 
                    (struct sockaddr *)&client_addr,
                    &client_len
            );

    // server_fd - listening socket
    // client_fd = communication socket

    if (client_fd < 0) {
        printf("\nError in Accepting Client\n");
        close(server_fd);
        exit(-1);
    }

    printf("Client connected!\n");


    // Receive filename
    char filename[FILENAME_SIZE];

    ssize_t filename_bytes = recv(client_fd, filename, sizeof(filename)-1, 0);

    if(filename_bytes < 0){
        printf("Server Error\n");
        close(server_fd);
        close(client_fd);
        exit(-1);
    }
    filename[filename_bytes] = '\0';

    printf("Receiving file: %s\n", filename);

    // create file
    char filepath[BUFFER_SIZE];

    // Same as path.join(STORAGE_DIR, filename) and store in filepath;
    snprintf(filepath, sizeof(filepath), "submissions/%s", filename);

    int file_fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(file_fd < 0){
        printf("Server Error");
        close(server_fd);
        close(client_fd);
        exit(-1);
    }

    // Receive file data
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    while((bytes_received = recv(client_fd,buffer,sizeof(buffer),0)) > 0){
        ssize_t bytes_written = write(file_fd,buffer,bytes_received);
        if(bytes_written < 0){
            printf("Write Error");
            close(file_fd);
            close(server_fd);
            close(client_fd);
            exit(-1);
        }
    }

    close(file_fd);
    printf("Source code received.\n");

    // compile source code
    char executable[512];
    snprintf(executable,sizeof(executable),"submissions/program");

    char compile_command[2048];
    snprintf(compile_command,sizeof(compile_command),"gcc %s -o %s 2> submissions/error.txt",
            filepath,executable);

    printf("Compiling...\n");

    int compile_result = system(compile_command);

    if(compile_result != 0){
        printf("Compilation failed. \n");

        int error_file_fd = open("submissions/error.txt",O_RDONLY);
        if(error_file_fd < 0){
            printf("Error in Opening a file");
            close(error_file_fd);
            close(server_fd);
            close(client_fd);
            exit(-1);
        }
        else{
            char error_buffer[BUFFER_SIZE];
            ssize_t error_bytes;
            error_bytes = read(error_file_fd,
                          error_buffer,
                          sizeof(error_buffer) - 1
                    );

            if (error_bytes < 0) {
                perror("read");
                close(server_fd);
                close(client_fd);
                close(error_file_fd);
                exit(-1);
            }

            error_buffer[error_bytes] = '\0';
            
            char response[BUFFER_SIZE];
            snprintf(
                response,
                sizeof(response),
                "COMPILATION ERROR\n\n%s",
                error_buffer
            );

            send(client_fd,response, strlen(response),0);
            close(error_file_fd);
        }

            close(client_fd);
            close(server_fd);

            return 0;
    }

    printf("Compilation successful.\n");

    printf("Running program...\n");


    char run_command[1024];
    snprintf(run_command,sizeof(run_command),"%s > submissions/output.txt 2>&1", executable);

    // 2>&1 means: stderr -> stdout
    // So both normal output and errors go into output.txt.

    int run_result = system(run_command);
    int output_file_fd = open("submissions/output.txt",O_RDONLY);

    if(output_file_fd < 0){
        perror("fopen");
        close(client_fd);
        close(server_fd);
        return 0;
    }

    char output[BUFFER_SIZE];
    ssize_t output_bytes = read(output_file_fd,output,sizeof(output)-1);
    output[output_bytes] = '\0';

    close(output_file_fd);

    char response[8192];

    if(run_result == 0){
        snprintf(
            response,
            sizeof(response),
            "COMPILED SUCCESSFULLY\n\nPROGRAM OUTPUT:\n%s",
            output
        );
    }
    else{
        snprintf(
            response,
            sizeof(response),
            "PROGRAM TERMINATED WITH ERROR\n\nOUTPUT:\n%s",
            output
        );
    }

    send(client_fd, response, strlen(response), 0);

    printf("Result sent to client.\n");

    close(client_fd);
    close(server_fd);

    return 0;
}
