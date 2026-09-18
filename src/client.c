#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#define PORT 8000
#define BUFFER_SIZE 4096 // common 4KB block size in OS
#define FILENAME_SIZE 256


int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Usage: %s <source_file.c>\n", argv[0]);
        exit(-1);
    }

    char *filepath = argv[1];
    // create socket
    int client_fd = socket(AF_INET,SOCK_STREAM,0);

    if(client_fd < 0){
        printf("\nSocket Error\n");
        exit(-1);
    }

    printf("Socket created successfully\n");

    // Configure server address - "Which server do I want to connect to?""
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Server is running on our own machine
    // inet_pton - It converts an IP address written as a human-readable string into the binary format required by the socket API.
    if(inet_pton(AF_INET,"127.0.0.1", &server_addr.sin_addr) <= 0){
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
    if (connect(client_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {

        printf("Connection Error");
        close(client_fd);
        exit(-1);
    }

    printf("Connected to server\n");

    // OPEN file
    int file_fd = open(filepath,O_RDONLY);

    if(file_fd < 0){
        printf("Server Error");
        close(client_fd);
        exit(-1);
    }

    printf("Opened file: %s\n", filepath);

    // extract filename
    const char *filename = strrchr(filepath, '/');
    if (filename != NULL){
        filename++;
    }
    else{
        filename = filepath;
    }

    printf("Sending filename: %s\n", filename);
    if(send(client_fd,filename,strlen(filename),0) < 0){
        perror("send filename");
        close(file_fd);
        close(client_fd);
        exit(-1);
    }

    // read and send file
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while((bytes_read = read(file_fd,buffer,sizeof(buffer))) > 0){
        ssize_t bytes_sent = send(client_fd,buffer,bytes_read,0);
        if(bytes_sent < 0){
            printf("Sent Error");
            close(file_fd);
            close(client_fd);
            exit(-1);
        }
    }

    if (bytes_read < 0){
        perror("read");
        close(file_fd);
        close(client_fd);
        exit(-1);
    }

    printf("Source code sent.\n");
    shutdown( client_fd, SHUT_WR );


    char response[BUFFER_SIZE];
    ssize_t response_bytes = recv(client_fd,response,sizeof(response)-1,0);

    if (response_bytes < 0){
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

    return EXIT_SUCCESS;

}