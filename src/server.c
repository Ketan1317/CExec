#include <stdio.h>
#include <stdlib.h> // Standard General Utilities
#include <string.h>

#include <unistd.h> // linux commands - close()

// to define functions and types for manipulating internet addresses and byte order.
#include <arpa/inet.h> // sockaddr_in, htons()
#include <sys/socket.h> // socket(), bind(), listen(), accept()

#define PORT 8000
#define BUFFER_SIZE 1024

int main(void){
    // create a tcp socket
    // AF_INET - Use IPv4 addresses
    // SOCK_STREAM - Use a stream-oriented socket
    // 0 - Choose the appropriate protocol automatically.
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd < 0){
        printf("\nError in creating Socket\n");
        exit(-1); 
    }

    printf("Socket created Successfully\n");

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

    const char *message = "Hello from server\n";
    send(client_fd,message,strlen(message),0);

    close(client_fd);
    close(server_fd);

    printf("Server stopped.\n");

    return 0;


}
