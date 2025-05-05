#include <stdio.h>      
#include <stdlib.h>     // exit, malloc, atoi
#include <string.h>     // strlen, strcmp, sscanf, memmove
#include <unistd.h>     // close
#include <arpa/inet.h>  // htons, htonl
#include <sys/types.h>  // basic socket types
#include <sys/socket.h> // socket functions: socket, bind, listen, accept
#include <netinet/in.h> // struct sockaddr_in

#define buffer_size 3000 // Define buffer size for reading data

// Function to serve a requested file to the client
int serve_file(int client_socket, char* file_path) {
    FILE* fp = fopen(file_path, "rb"); // Open the requested file in binary read mode
    char buffer[buffer_size];
    int bytes_read;

    if (fp == NULL) {
        // If the file doesn't exist, send a 404 Not Found HTTP response
        const char* message = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
        send(client_socket, message, strlen(message), 0);
        return -1;
    }

    // Send a 200 OK HTTP header to the client
    const char *header = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_socket, header, strlen(header), 0);

    // Read the file in chunks and send it to the client
    while ((bytes_read = fread(buffer, 1, buffer_size, fp)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(fp); // Close the file
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        // Check if port number is provided
        printf("Usage: %s <port>\n", argv[0]);
        exit(-1);
    }

    char buffer[buffer_size];
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);
    char method[8], path[256]; // Buffers to hold HTTP method and path

    // Create a TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(-1);
    }

    // Set up the server address struct
    server_address.sin_family = AF_INET;                  // Use IPv4
    server_address.sin_port = htons(atoi(argv[1]));       // Convert port to network byte order
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);   // Accept connections from any IP

    // Bind the socket to the specified port and IP
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        exit(-1);
    }

    // Start listening for incoming connections (up to 3 pending)
    if (listen(server_socket, 3) < 0) {
        perror("listen");
        exit(-1);
    }

    printf("Server listening on %s\n", argv[1]);

    // Main server loop: accept and handle incoming connections
    while (1) {
        // Accept a new connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
        if (client_socket < 0) {
            perror("accept");
            exit(-1);
        }

        // Receive the HTTP request
        recv(client_socket, buffer, buffer_size, 0);

        // Parse the HTTP method and path (e.g., "GET /index.html")
        sscanf(buffer, "%s %s", method, path);

        // Remove the leading '/' from the path to form a valid filename
        memmove(path, path + 1, strlen(path));

        // Attempt to serve the requested file
        serve_file(client_socket, path);

        // Close the connection with the client
        close(client_socket);
    }

    // Close the server socket (unreachable code in this infinite loop)
    close(server_socket);
}
