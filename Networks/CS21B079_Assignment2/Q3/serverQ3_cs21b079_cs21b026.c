#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
//Whenever we are updating the client_count we have to use mutex_lock, it's involved in the critical section.

// Structure to hold client information
typedef struct {
    int socket_fd;
    char username[50];
} ClientInfo;

// Global variables
ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;
int timeout_seconds;

///broadcast is used to share a message among all the clients.
void broadcast(char *message, int sender_socket_fd) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket_fd != sender_socket_fd) {
            send(clients[i].socket_fd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

// Function to handle a client
void *handle_client(void *arg) {
    int client_index = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    // Set timeout for the client socket
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    setsockopt(clients[client_index].socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));

    // Receive and broadcast messages
    while ((bytes_received = recv(clients[client_index].socket_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0';

        // Check if it's a special command(\list,\bye)
        if (strncmp(buffer, "\\list", 5) == 0) {
            char userlist[BUFFER_SIZE];
            strcpy(userlist, "Currently in the chat room:\n");
            for (int i = 0; i < client_count; i++) {
                strcat(userlist, clients[i].username);
                strcat(userlist, "\n");
            }
            send(clients[client_index].socket_fd, userlist, strlen(userlist), 0);
            continue;
        } else if (strncmp(buffer, "\\bye", 4) == 0) {
            char leave_message[BUFFER_SIZE + 50];
            sprintf(leave_message, "Client %s left the chatroom\n", clients[client_index].username);
            broadcast(leave_message, clients[client_index].socket_fd);
            printf("Client %s left the chatroom\n", clients[client_index].username);
            close(clients[client_index].socket_fd);
            pthread_mutex_lock(&mutex);
            for (int i = client_index; i < client_count - 1; ++i) {
                clients[i] = clients[i + 1];
            }
            client_count--;
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        char message[BUFFER_SIZE + 50];
        sprintf(message, "%s: %s", clients[client_index].username, buffer);
        //broadcast is used to share a message among all the clients.
        broadcast(message, clients[client_index].socket_fd);
        printf("%s: %s\n", clients[client_index].username, buffer);
    }

    // If recv returns 0, client closed connection; if it returns -1, there was an error or timeout
    if (bytes_received == 0) {
        printf("Client %s left the chatroom\n", clients[client_index].username);
        char leave_message[BUFFER_SIZE + 50];
        sprintf(leave_message, "Client %s left the chatroom\n",clients[client_index].username);
        broadcast(leave_message, clients[client_index].socket_fd);
    } else if (bytes_received == -1) {
        printf("Client %s timed out\n", clients[client_index].username);
        send(clients[client_index].socket_fd, "You are kicked out of the room as the time given to you is finished :((\n", strlen("You are kicked out of the room as the time given to you is finished :((\n"), 0);
        char leave_message[BUFFER_SIZE + 50];
        sprintf(leave_message, "Client %s kicked out of the chatroom\n",clients[client_index].username);
        broadcast(leave_message, clients[client_index].socket_fd);
    }

    // Since the client is either kicked out or clinet is closed, we have to remove that client from the list we have.
    close(clients[client_index].socket_fd);
    pthread_mutex_lock(&mutex);
    for (int i = client_index; i < client_count - 1; ++i) {
        clients[i] = clients[i + 1];
    }
    client_count--;
    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <port> <max_clients> <timeout_seconds>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int max_clients = atoi(argv[2]);
    timeout_seconds = atoi(argv[3]);

    int server_socket_fd, client_socket_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd < 0) {
        perror("Error in socket creation");
        return 1;
    }

// Setting server address details
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // / Binding socket to address and port specified in the command line argument
    if (bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in binding");
        return 1;
    }

    // Now we'll start listening for incoming connections
    if (listen(server_socket_fd, max_clients) < 0) {
        perror("Error in listen");
        return 1;
    }

    printf("Server started on port %d\n", port);

    while (1) {
        // accepting a client request and checking it's status - successfull or not
        client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket_fd < 0) {
            perror("Error in accept");
            continue;
        }

        // Receive username from client
        char username[50];
        memset(username, '/0', sizeof(username));
        // at first we have send 3 times but understood that we should not do that because set_user_name from client code has only one read and then there is send, So we put everything is one send before recv from client.
         if (client_count == 0) {
            send(client_socket_fd, "Welcome to the chat room!\nroom is empty :(\nEnter your username that is not present in the room: \n", strlen("Welcome to the chat room!\nroom is empty :(\nEnter your username that is not present in the room: \n"), 0);
        } else {
            char userlist[1024];
            strcpy(userlist, "Welcome to the chat room!\nCurrently in the room:\n");
            for (int i = 0; i < client_count; i++) {
                strcat(userlist, clients[i].username);
                strcat(userlist, "\n");
            }
            strcat(userlist,"Enter your username that is not present in the room: \n");
            send(client_socket_fd, userlist, strlen(userlist), 0);
        }


        // recieve the username from the client
        int data_size_recieved = recv(client_socket_fd, username, sizeof(username), 0);
        // make the end character as \0, at first we didn't and then we faced the issue of getting wrong client_names.
        username[data_size_recieved] = '\0';
        // Check if maximum number of clients reached
        if (client_count >= max_clients) {
            send(client_socket_fd, "Chatroom is full. Try again later.\n", strlen("Chatroom is full. Try again later.\n"), 0);
            close(client_socket_fd);
            continue;
        }

        //If the username chosen already exists then we have to print "Username already exists. Try again with a different username.\n", strlen("Username already exists. Try again with a different username.".
        int exists = 0;
        for (int i = 0; i < client_count; ++i) {
            if (strcmp(clients[i].username, username) == 0) {
                exists = 1;
                break;
            }
        }
        if (exists) {
            send(client_socket_fd, "Username already exists. Try again with a different username.\n", strlen("Username already exists. Try again with a different username.\n"), 0);
            close(client_socket_fd);
            continue;
        }
        

        // Add client to the list
        pthread_mutex_lock(&mutex);
        clients[client_count].socket_fd = client_socket_fd;
        strcpy(clients[client_count].username, username);
        client_count++;
        pthread_mutex_unlock(&mutex);

        // Create thread to handle client
        pthread_t thread;
        int *arg = (int *)malloc(sizeof(*arg));
        *arg = client_count - 1;
        if (pthread_create(&thread, NULL, handle_client, arg) != 0) {
            perror("Error in creating thread");
            close(client_socket_fd);
        }

        // Notify other clients about new client using the function broadcast.
        char message[BUFFER_SIZE];
        sprintf(message, "%s joined the chatroom\n", username);
        broadcast(message, client_socket_fd);
        printf("%s joined the chatroom\n", username);
    }

    // Close server socket
    close(server_socket_fd);

    return 0;
}