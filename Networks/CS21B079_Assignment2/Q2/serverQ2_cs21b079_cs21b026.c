#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define DEFAULT_HTML "index.html"
#define NOT_FOUND_HTML "404.html"

struct ThreadArgs
{
    int arg1;
    char *arg2;
    char *arg3;
    char *arg4;
};

const char *root_directory;

char *substring(char *s, int i, int j)
{
    int length = j - i + 1;
    char *sub = (char *)malloc((length + 1) * sizeof(char));
    strncpy(sub, s + i, length);
    sub[length] = '\0';
    return sub;
}

char *modifing_string(char *ip) // here we are returning the atring between %**%
{
    char *ret;
    int l = strlen(ip);
    int s = -1, e = l;
    int cs = 0, ce = 0;
    for (int i = 0; i < l; i++)
    {
        if (ip[i] == '%')
        {
            if (cs == 1)
            {
                s = i;
                cs = 2;
            }
            else if (cs == 0)
            {
                cs = 1;
            }
            else
            {
                cs = 2;
            }
        }
    }
    for (int i = l - 1; i >= 0; i--)
    {
        if (ip[i] == '%')
        {
            if (ce == 1)
            {
                e = i;
                ce = 2;
            }
            else if (ce == 0)
            {
                ce = 1;
            }
            else
            {
                ce = 2;
            }
        }
    }
    ret = substring(ip, s + 1, e - 1);
    return ret;
}

// handle_post_request is the function which we using to handle all the POST requesta from the client
void handle_POST_request(int client_socket, char *request)
{
    char *content_length_header = strstr(request, "Content-Length:");
    if (content_length_header == NULL)
    {
        perror("Content-Length header not found");
        close(client_socket);
        return;
    }

    char *mod_data = modifing_string(request); // getting the required message from the request by selecting the region between first and last "%**%"

    // Counting characters, words, and sentences

    int noof_chars = strlen(mod_data);
    int noof_sentences = 0;
    for (int i = 0; i < noof_chars; i++)
    {
        if (mod_data[i] == '.' || mod_data[i] == '?' || mod_data[i] == '!')
        {
            noof_sentences = noof_sentences + 1;
        }
    }
    noof_sentences = noof_sentences + 1;
    for (int i = noof_chars - 1; i >= 0; i--)
    {
        if (!isspace(mod_data[i]))
        {
            if (mod_data[i] == '.' || mod_data[i] == '?' || mod_data[i] == '!')
            {
                noof_sentences = noof_sentences - 1;
            }
            break;
        }
    }
    int noof_lines = 1;
    for (int i = 0; i < noof_chars; i++)
    {
        if (mod_data[i] == '\n')
        {
            noof_lines = noof_lines + 1;
        }
    }
    int noof_words = 0;
    int isnew_word = 1;
    for (int i = 0; i < noof_chars; i++)
    {
        if (isspace(mod_data[i]) || mod_data[i] == '.' || mod_data[i] == '?' || mod_data[i] == '!')
        {
            isnew_word = 1;
        }
        else
        {
            if (isnew_word)
            {
                noof_words++;
                isnew_word = 0;
            }
        }
    }
    // Construct response
    char response[BUFFER_SIZE];
    snprintf(response, BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                                    "<html><head><title>POST Request Results</title></head><body>"
                                    "<h1>POST Request Results</h1>"
                                    "<p>Number of characters: %d</p>"
                                    "<p>Number of words: %d</p>"
                                    "<p>Number of sentences: %d</p>"
                                    "</body></html>",
             noof_chars - noof_lines + 1, noof_words, noof_sentences);

    // Send response back to the client
    send(client_socket, response, strlen(response), 0);
    close(client_socket);
}

// handle_get_request is the function which we using to handle all the GET requesta from the client
void handle_GET_request(int client_socket, const char *requested_resource, const char *root_directory)
{
    // getting the file path of the requested resourse from the root directory + requestd resource
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/%s", root_directory, requested_resource);

    // checking if their exista a file in the file path obtained above
    if (access(file_path, F_OK) != -1) // File exists,
    {
        const char *mime_type = "text/plain"; // Default MIME type
        const char *file_extension = strrchr(requested_resource, '.');
        // determine the MIME type based on file extension
        if (file_extension != NULL)
        {
            if (strcmp(file_extension, ".html") == 0)
            {
                mime_type = "text/html";
            }
            else if (strcmp(file_extension, ".css") == 0)
            {
                mime_type = "text/css";
            }
            else if (strcmp(file_extension, ".js") == 0)
            {
                mime_type = "application/javascript";
            }
            else if (strcmp(file_extension, ".jpg") == 0 || strcmp(file_extension, ".jpeg") == 0)
            {
                mime_type = "image/jpeg";
            }
            else if (strcmp(file_extension, ".png") == 0)
            {
                mime_type = "image/png";
            }
        }

        // Open and send the file content with appropriate MIME type
        FILE *file = fopen(file_path, "r");
        if (file != NULL)
        {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read;
            // sending the HTTP header with MIME type
            snprintf(buffer, BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", mime_type);
            send(client_socket, buffer, strlen(buffer), 0);
            // now we are sending the file contents
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
            {
                send(client_socket, buffer, bytes_read, 0);
            }
            fclose(file);
        }
    }
    else // If the file requested is not present then we are sending the 404.html file
    {
        char error_404[256];
        snprintf(error_404, sizeof(error_404), "%s/%s", root_directory, NOT_FOUND_HTML);
        FILE *not_found_file = fopen(error_404, "r"); // here not_found_file contains data of 404.html file
        if (not_found_file != NULL)
        {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read;
            // sending the HTTP header with text/html MIME type
            snprintf(buffer, BUFFER_SIZE, "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n");
            send(client_socket, buffer, strlen(buffer), 0); // Sending not_found_file i.e., 404 page content
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, not_found_file)) > 0)
            {
                send(client_socket, buffer, bytes_read, 0);
            }
            fclose(not_found_file);
        }
    }
    close(client_socket);
}

void *handle_client(void *obj)
{
    // handle_client is the function which we using for thread creation 

    int client_fd = ((struct ThreadArgs *)obj)->arg1;
    char *requested_resource = ((struct ThreadArgs *)obj)->arg3;
    char *method = ((struct ThreadArgs *)obj)->arg4;
    char *buffer = ((struct ThreadArgs *)obj)->arg2;

    if (strcmp(method, "GET") == 0)
    {
        handle_GET_request(client_fd, requested_resource, root_directory);
    }
    else if (strcmp(method, "POST") == 0)
    {
        handle_POST_request(client_fd, buffer);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <port> <root_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);
    root_directory = argv[2];

    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Creating a TCP socket and checking weather it is sucessfully created or not
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setting server address details
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Binding socket to address and port specified in the command line argument
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Now we'll start listening for incoming connections
    if (listen(server_fd, 10) < 0)
    {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1)
    {
        // accepting a client request and checking it's status - successfull or not
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Acceptance failed");
            exit(EXIT_FAILURE);
        }

        // Read client request
        read(client_fd, buffer, BUFFER_SIZE);

        // Extract request method and requested resource from the request
        char method[8], requested_resource[256];
        sscanf(buffer, "%s /%s", method, requested_resource);

        // If no specific resource is requested, default to index.html
        if (strlen(requested_resource) == 0)
        {
            strcpy(requested_resource, DEFAULT_HTML);
        }

        // creating threads to accomodate multiple requests
        pthread_t thread;
        struct ThreadArgs *obj = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
        obj->arg1 = client_fd;
        obj->arg2 = buffer;
        obj->arg3 = requested_resource;
        obj->arg4 = method;

        if (pthread_create(&thread, NULL, &handle_client, obj) != 0)
        {
            perror("Thread creation failed");
            close(client_fd);
        }
    }

    return 0;
}
