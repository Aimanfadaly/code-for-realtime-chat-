#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 5000
#define MAX_NAME_LENGTH 32
#define MAX_CLIENTS 10

typedef struct {
    int socket;
    struct sockaddr_in address;
    int addr_len;
    char name[MAX_NAME_LENGTH];
} client_t;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            if (clients[i]->socket != sender_socket) {
                if (send(clients[i]->socket, message, strlen(message), 0) < 0) {
                    perror("send");
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *client_ptr) {
    client_t *client = (client_t *)client_ptr;
    char buffer[BUFFER_SIZE];
    int nbytes;

    while ((nbytes = recv(client->socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[nbytes] = '\0';

        char message[BUFFER_SIZE + MAX_NAME_LENGTH];
        sprintf(message, "%s: %s", client->name, buffer);
        printf("%s\n", message);

        broadcast_message(message, client->socket);
    }

    close(client->socket);
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client) {
            clients[i] = NULL;
            printf("%s has disconnected from the server\n", client->name);
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    free(client);
    return NULL;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);
    pthread_t tid;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(1);
    }

    // Set up server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // Listen on all available network interfaces
    server_address.sin_port = htons(SERVER_PORT);

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("bind");
        close(server_socket);
        exit(1);
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_socket);
        exit(1);
    }

    printf("Server is listening on port %d\n", SERVER_PORT);

    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        // Create client structure
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->socket = new_socket;
        client->address = client_address;
        client->addr_len = client_len;

        // Receive client name
        char name[MAX_NAME_LENGTH];
        if (recv(new_socket, name, MAX_NAME_LENGTH, 0) <= 0) {
            perror("recv");
            continue;
        }
        strcpy(client->name, name);

        // Print client connection message
        printf("%s has connected to the server\n", client->name);

        // Add client to the client list
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i]) {
                clients[i] = client;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Create a thread to handle the client
        pthread_create(&tid, NULL, handle_client, (void *)client);
    }

    close(server_socket);
    return 0;
}
