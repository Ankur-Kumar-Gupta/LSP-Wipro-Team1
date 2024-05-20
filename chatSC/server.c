#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>

#define PORT 3333
#define SIZE 1024
#define MAX_CLIENTS 10

sem_t sem1, sem2;
char buffer[SIZE];

typedef struct {
    struct sockaddr_in addr;
    int active;
    int id;
} ClientInfo;

typedef struct {
    char from[INET_ADDRSTRLEN];
    char to[INET_ADDRSTRLEN];
    char msg[SIZE];
} Msgs;

ClientInfo clients[MAX_CLIENTS];

void *Requests(void *arg);
void list_users();
void broadcast_message(int , const char *);
void disconnect_client(int );
void servershell(int );


int main() {
    int serversock;
    struct sockaddr_in server_addr;
    pthread_t tid;

    if (sem_init(&sem1, 0, 1) != 0) {
        perror("Semaphore sem1 initialization failed");
        exit(1);
    }

    if (sem_init(&sem2, 0, 0) != 0) {
        perror("Semaphore sem2 initialization failed");
        exit(1);
    }

    serversock = socket(AF_INET, SOCK_DGRAM, 0);
    if (serversock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(serversock, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind failed");
        exit(1);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
    }

    printf("Server is running on port %d.\n", PORT);

    pthread_create(&tid, NULL, Requests, (void *)&serversock);

    servershell(serversock);

    close(serversock);

    sem_destroy(&sem1);
    sem_destroy(&sem2);

    return 0;
}


void *Requests(void *arg) {
    int serversock = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    char Data[] = "Message Received ";

    while (1) {
        addr_len = sizeof(client_addr);
        ssize_t message_len = recvfrom(serversock, buffer, SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);

        printf("\n Received message from %s: %d: %s\n\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);

        int client_count = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && 
                clients[i].addr.sin_addr.s_addr == client_addr.sin_addr.s_addr && 
                clients[i].addr.sin_port == client_addr.sin_port) {
                client_count = i;
                break;
            }
        }

        if (client_count == -1) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) {
                    clients[i].addr = client_addr;
                    clients[i].active = 1;
                    clients[i].id = i;
                    client_count = i;
                    break;
                }
            }
        }
        if (client_count != -1) {
            
            int target_id;
            char msg[SIZE];
            sscanf(buffer, "%d:%[^\n]", &target_id, msg);

           
            Msgs message;
            strcpy(message.from, inet_ntoa(client_addr.sin_addr));
            snprintf(message.to, INET_ADDRSTRLEN, "%d", target_id);
            strncpy(message.msg, msg, SIZE);

           
            if (target_id >= 0 && target_id < MAX_CLIENTS && clients[target_id].active) {
                sendto(serversock, msg, strlen(msg), 0, (struct sockaddr *)&clients[target_id].addr, sizeof(clients[target_id].addr));
            } else {
                
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].active && i != client_count) {
                        sendto(serversock, msg, strlen(msg), 0, (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr));
                    }
                }
            }
        }

        sendto(serversock, Data, strlen(Data), 0, (struct sockaddr *)&client_addr, addr_len);
    }

    pthread_exit(NULL);
}

void list_users() {
    printf("Connected clients: \n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            printf("Client %d - %s:%d\n", clients[i].id, inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
        }
    }
}

void broadcast_message(int serversock, const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            sendto(serversock, message, strlen(message), 0, (struct sockaddr *)&clients[i].addr, sizeof(clients[i].addr));
        }
    }
}

void disconnect_client(int client_id) {
    if (client_id >= 0 && client_id < MAX_CLIENTS && clients[client_id].active) {
        clients[client_id].active = 0;
        printf("Client %d disconnected.\n", client_id);
    } else {
        printf("Invalid client ID.\n");
    }
}

void servershell(int serversock) {
    char command[SIZE];
    while (1) {
       // printf("\nEnter command: ");
        fgets(command, SIZE, stdin);
        command[strcspn(command, "\n")] = 0; 

        if (strncmp(command, "list users", 10) == 0) {
            list_users();
        } else if (strncmp(command, "broadcast", 9) == 0) {
            char *message = command + 10;
            broadcast_message(serversock, message);
        } else if (strncmp(command, "kill", 4) == 0) {
            int client_id = atoi(command + 5);
            disconnect_client(client_id);
        } else {
            printf("Unknown command.\n");
        }
    }
}
