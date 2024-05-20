#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 3333
#define SIZE 1024

void *receive_messages(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[SIZE];

    while (1) {
        ssize_t recv_len = recvfrom(client_socket, buffer, SIZE, 0, NULL, NULL);
        if (recv_len > 0) {  
            printf("Server response: %s\n", buffer);
        }
    }
    pthread_exit(NULL);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[SIZE];
    pthread_t t_id;

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        printf("Socket creation failed");
        exit(0);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    pthread_create(&t_id, NULL, receive_messages, (void *)&client_socket);

    while (1) {
        printf("Enter client ID and message ");
        fgets(buffer, SIZE, stdin);
        sendto(client_socket, (const char *)buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        usleep(100000);
    }

    close(client_socket);

    return 0;
}
