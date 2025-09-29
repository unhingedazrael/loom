
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 3000  // buffer size

void banana(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int serve_file(int client_socket, const char* file_path) {
    FILE* filepointer = fopen(file_path, "rb");
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    if (!filepointer) {
        const char* message =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 13\r\n\r\n"
            "404 Not Found";

        send(client_socket, message, strlen(message), 0);
        return -1;
    }

    const char* header = "HTTP/1.1 200 OK\r\n\r\n";
    send(client_socket, header, strlen(header), 0);

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, filepointer)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(filepointer);
    return EXIT_SUCCESS;
}

void* client_thread(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE], method[8], path[256];

    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        if (sscanf(buffer, "%7s %255s", method, path) == 2) {
            memmove(path, path + 1, strlen(path)); // strip leading '/'
            serve_file(client_socket, path);
        }
    }

    close(client_socket);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        banana("missing port");
    }

    int server_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) banana("socket");

    int option = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
        banana("bind");

    if (listen(server_socket, 10) < 0)
        banana("listen");

    printf("Server listening on port %s\n", argv[1]);

    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_len);
        if (client_socket < 0) banana("accept");

        int *client_pointer = malloc(sizeof(int));
        *client_pointer = client_socket;

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, client_pointer);
        pthread_detach(tid);
    }

    close(server_socket);
    return EXIT_SUCCESS;
}
