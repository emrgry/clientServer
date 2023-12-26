#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8081
#define MAX_USERS 10

void *handle_client(void *socket_fd)
{
    int new_socket = *(int *)socket_fd;
    char buffer[1024] = {0};
    const char *hello = "Hello from server";

    read(new_socket, buffer, 1024);
    printf("Client %d: %s\n", new_socket, buffer);

    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent to client %d\n", new_socket);

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
            perror("Accepting new client error");
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
