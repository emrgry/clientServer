#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8081
#define MAX_USERS 10

typedef struct
{
    int type; // -1 for disconnect, 0 for login request, 1 for regular message
    char body[1024];
    int to;   // -1 for server, user_id for specific user
    int from; // -1 for server, user_id for specific user
} Message;

int user_map[MAX_USERS]; // Array to map socket numbers to user IDs

void notifyClientsAndShutdown()
{
    Message disconnectMessage;
    disconnectMessage.type = -1; // -1 indicates a disconnect message

    for (int i = 0; i < MAX_USERS; i++)
    {
        if (user_map[i] != -1) // if there is a client connected on this socket
        {
            disconnectMessage.from = user_map[i];
            send(i, &disconnectMessage, sizeof(disconnectMessage), 0);
            close(i);
            user_map[i] = -1;
        }
    }

    close(0);
}

void disconnectClient(int new_socket, int user_map[])
{
    int user_id = user_map[new_socket];
    printf("Client with user_id %d disconnected\n", user_id);
    close(new_socket);
    user_map[new_socket] = -1; // Remove the user from the map
    pthread_exit(NULL);
}

void *handle_client(void *socket_fd)
{
    int new_socket = *(int *)socket_fd;
    Message received_message;

    while (1)
    {
        int valrec = recv(new_socket, &received_message, sizeof(received_message), 0);
        if (valrec <= 0)
        {
            disconnectClient(new_socket, user_map);
        }
        if (received_message.type == -1) // disconnect request
        {
            disconnectClient(new_socket, user_map);
        }
        else if (received_message.type == 0) // login request
        {
            printf("Login request received from client: %d userId: %d\n", new_socket, received_message.from);
            // Save user_id to a file
            FILE *file = fopen("user_list.txt", "a");
            if (file != NULL)
            {
                printf("Saving user_id to file\n");
                fprintf(file, "%d\n", received_message.from);
                fclose(file);
            }
            else
            {
                printf("Error opening file\n");
            }
            // Store the mapping from socket number to user_id
            user_map[new_socket] = received_message.from;
        }
        else if (received_message.type == 1)
        {
            printf("Message from client %d: %s\n", new_socket, received_message.body);
        }
        else
        {
            printf("Client %d: %s\n", new_socket, received_message.body);
        }
    }

    // char buffer[1024] = {0};

    // read(new_socket, buffer, 1024);
    // strcat(buffer, " is online");

    // send(new_socket, buffer, strlen(buffer), 0);
    // printf("Message sent to client %d: %s\n", new_socket, buffer);

    close(new_socket);
    pthread_exit(NULL);
}

int main()
{

    int clients[MAX_USERS] = {0};
    pthread_t threads[MAX_USERS];
    int thread_count = 0;

    int server_sock = 0;
    struct sockaddr_in serv_addr;
    int addrlen = sizeof(serv_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Server Socket Failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // bind the socket
    if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Binding Server socket err");
        exit(EXIT_FAILURE);
    }

    // listen the prot
    if (listen(server_sock, 5) < 0)
    {
        perror("server listen err");
        exit(EXIT_FAILURE);
    }

    atexit(notifyClientsAndShutdown);

    while (1)
    {
        int new_client = accept(server_sock, (struct sockaddr *)&serv_addr, (socklen_t *)&addrlen);
        if (new_client < 0)
        {
            perror("Error! When server accepting new client");
            exit(EXIT_FAILURE);
        }

        printf("\nnew client connected with client id: %d\n", new_client);

        clients[thread_count] = new_client;
        int thread_create = pthread_create(&threads[thread_count], NULL, handle_client, (void *)&clients[thread_count]);
        if (thread_create < 0)
        {
            perror("thread create for client error");
            exit(EXIT_FAILURE);
        }

        thread_count++;
        if (thread_count >= MAX_USERS)
        {
            printf("too many clients.Abort new connections\n");
            close(new_client);
        }
    }

    return 0;
}
