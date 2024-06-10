#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
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
#else
typedef int SocketType;
#endif

void *receive_messages(void *socket_desc) {
    SocketType clientSocket = *(SocketType *)socket_desc;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

#ifdef _WIN32
        if (recv(clientSocket, buffer, BUFFER_SIZE, 0) > 0) {
#else
        if (recv(clientSocket, buffer, BUFFER_SIZE, MSG_DONTWAIT) > 0) {
#endif
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

    SocketType clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char clientName[MAX_NAME_LENGTH];
    pthread_t recvThread;

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        perror("socket");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("SERVER_IP_ADDRESS"); // Replace with server IP address
    serverAddr.sin_port = htons(SERVER_PORT);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Get client name
    printf("Enter your name: ");
    fgets(clientName, MAX_NAME_LENGTH, stdin);
    clientName[strcspn(clientName, "\n")] = '\0';

    // Send client name to the server
    if (send(clientSocket, clientName, strlen(clientName), 0) < 0) {
        perror("send");
        close(clientSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Create a thread to receive messages from the server
    if (pthread_create(&recvThread, NULL, receive_messages, (void *)&clientSocket) < 0) {
        perror("pthread_create");
        close(clientSocket);
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

        send(clientSocket, buffer, strlen(buffer), 0);
    }

    close(clientSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
