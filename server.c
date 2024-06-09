#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

#define BUFFER_SIZE 1024
#define SERVER_PORT 5000
#define MAX_NAME_LENGTH 32

#ifdef _WIN32
typedef SOCKET SocketType;
typedef HANDLE pthread_t; // Define pthread_t as HANDLE for Windows
#else
typedef int SocketType;
#endif

#ifdef _WIN32
// Wrapper function for creating threads on Windows
int pthread_create_win(pthread_t *thread, const void *unused, void *(*start_routine)(void *), void *arg) {
    DWORD threadId;
    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, &threadId);
    return (*thread != NULL) ? 0 : 1;
}
#define pthread_create pthread_create_win
#define close closesocket // Use closesocket() instead of close() on Windows
#endif

void *receive_messages(void *socket_desc) {
    SocketType client_socket = *(SocketType *)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        if (recv(client_socket, buffer, BUFFER_SIZE, 0) > 0) {
            printf("\r%s\n> ", buffer);
            fflush(stdout);
        }

#ifdef _WIN32
        Sleep(100); // Sleep for 100 milliseconds to prevent high CPU usage on Windows
#endif
    }

    return NULL;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup");
        exit(EXIT_FAILURE);
    }
#endif

    SocketType client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    char name[MAX_NAME_LENGTH];
    pthread_t receive_thread;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == -1) {
        perror("socket");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(SERVER_PORT);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connect");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Get client name
    printf("Enter your name: ");
    fgets(name, MAX_NAME_LENGTH, stdin);
    name[strcspn(name, "\n")] = '\0';

    // Send client name to the server
    if (send(client_socket, name, strlen(name), 0) < 0) {
        perror("send");
        close(client_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Create a thread to receive messages from the server
    if (pthread_create(&receive_thread, NULL, receive_messages, (void *)&client_socket) != 0) {
        perror("pthread_create");
        close(client_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "quit") == 0) {
            break;
        }

        send(client_socket, buffer, strlen(buffer), 0);
    }

    close(client_socket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
