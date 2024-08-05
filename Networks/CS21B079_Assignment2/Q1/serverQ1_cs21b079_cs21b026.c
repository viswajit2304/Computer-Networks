#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <libgen.h>

#define BUFFER_SIZE 1024

int active_connections = 0; // total no of client connections at a instance
int max_connections;        // maximum no of client connections at a time // given as a command line argument

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs
{ // structure to pass as a argument to threads
    int arg1;
    const char *arg2;
};

void *handle_client(void *obj)
{ // handle_client is the function which we are using for thread creation
    int client_socket = ((struct ThreadArgs *)obj)->arg1;
    const char *music_directory = ((struct ThreadArgs *)obj)->arg2;

    char song_number_str[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read song number from client
    bytes_read = recv(client_socket, song_number_str, sizeof(song_number_str), 0);
    if (bytes_read <= 0)
    {
        perror("Error reading song number");
        close(client_socket);
        return NULL;
    }
    song_number_str[bytes_read] = '\0';

    // Convert song number string to integer
    int song_number = atoi(song_number_str);
    if (song_number < 1 || song_number > 10)
    {
        perror("Invalid song number");
        close(client_socket);
        return NULL;
    }

    // Construct file path to the corresponding song
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%d.mp3", music_directory, song_number);
    const char *mp3_file_name = basename(file_path);

    printf("Requested MP3 file name: %s\n", mp3_file_name); // printing the requested song name

    // Open the requested file
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1)
    {
        perror("Error opening file");
        close(client_socket);
        return NULL;
    }

    // Send file data to client
    char buffer[BUFFER_SIZE];
    ssize_t bytes_sent;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0)
    {
        bytes_sent = send(client_socket, buffer, bytes_read, 0);
        if (bytes_sent != bytes_read)
        {
            perror("Error sending file data");
            close(client_socket);
            close(file_fd);
            return NULL;
        }
    }

    // Close the file and client socket
    close(file_fd);
    close(client_socket);

    pthread_mutex_lock(&mutex);
    active_connections--;         // Decrement the active connections count
    pthread_mutex_unlock(&mutex); // Unlock the mutex after accessing shared resources

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <port> <music_directory> <max_streams>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int P = atoi(argv[1]);
    const char *DIR = argv[2];
    int N = atoi(argv[3]);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    // Creating a TCP socket and checking weather it is sucessfully created or not
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setting server address details
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(P);

    // Binding socket to address and port specified in the command line argument
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Now we'll start listening for incoming connections
    if (listen(server_fd, N) < 0)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", P);

    max_connections = N;

    while (1)
    {
        if (active_connections < max_connections)
        { // Accept a new connection if the active connections count is less than the maximum allowed

            // accepting a client request and checking it's status - successfull or not
            if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addrlen)) < 0)
            {
                perror("Acceptance failed");
                exit(EXIT_FAILURE);
            }

            // printing the client IP address after extracting it from client address
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            printf("Client connected from IP address: %s\n", client_ip);

            pthread_mutex_lock(&mutex);
            active_connections++;         // Incrementing the active_connections count
            pthread_mutex_unlock(&mutex); // Unlock the mutex after accessing shared resources

            // Create a thread for each client connection and to accomodate multiple requests
            pthread_t thread;

            struct ThreadArgs *obj = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
            obj->arg1 = client_fd;
            obj->arg2 = DIR;

            if (pthread_create(&thread, NULL, &handle_client, obj) != 0)
            {
                perror("Thread creation failed");
                close(client_fd);
            }
        }
        else
        {
            printf("Maximum connections reached. Rejecting new connections.\n");
            sleep(1); // Sleep to avoid busy-waiting
        }
    }

    close(server_fd);
    return 0;
}