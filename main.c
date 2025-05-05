```c
#include <stdio.h>      // printf, perror, fopen, fread, etc.
#include <stdlib.h>     // exit, malloc
#include <string.h>     // strlen, strcmp, sscanf, etc.
#include <unistd.h>     // close
#include <arpa/inet.h>  // inet_ntoa, htons, etc.
#include <fcntl.h>      // open (optional)
#include <sys/types.h>  // socket types
#include <sys/socket.h> // socket functions
#include <netinet/in.h> // sockaddr_in

int serve_file(int client_sock, char* file_path) {
    FILE* file = fopen(file_path, "rb");
    char buffer[3000];
    int bytes_read;

    if (file == NULL) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
        send(client_sock, not_found, strlen(not_found), 0);
        return 0;
    }

    const char *header = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_sock, header, strlen(header), 0);

    while ((bytes_read = fread(buffer, 1, 1024, file)) > 0) {
        send(client_sock, buffer, bytes_read, 0);
    }

    fclose(file);
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    char buffer[3000];
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char method[8], path[256];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock < 0) {
        perror("socket");
        return 0;
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 0;
        exit(1);
    }

    if (listen(server_sock, 3) < 0) {
        perror("listen");
        return 0;
        exit(1);
    }

    printf("Server running on port %s\n", argv[1]);

    while (1)
    {
        client_sock = accept(server_sock, (struct sockaddr*) &client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            return 0;
            exit(1);
        }

        recv(client_sock, buffer, sizeof(buffer), 0);
        sscanf(buffer, "%s %s", method, path);

        if (strcmp(path, "/") == 0) {
            strcpy(path, "/index.html");
        }

        memmove(path, path + 1, strlen(path));
        serve_file(client_sock, path);
        close(client_sock);
    }
    
    close(server_sock);

    return 1;
}```
