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
    int type; // 0 for login request, 1 for regular message
    char body[1024];
    int to;   // -1 for server, user_id for specific user
    int from; // -1 for server, user_id for specific user
} Message;

void *handle_client(void *socket_fd)
{
    int new_socket = *(int *)socket_fd;
    Message received_message;

    while (1)
    {
        int valrec = recv(new_socket, &received_message, sizeof(received_message), 0);
        if (received_message.type == 0) // login request
        {
            printf("Login request received from client: %d userId: %s\n", new_socket, received_message.from);
            // Save user_id to a file
            FILE *file = fopen("TerChatApp/users/user_list.txt", "a");
            if (file != NULL)
            {
                fprintf(file, "%s\n", received_message.from);
                fclose(file);
            }
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
