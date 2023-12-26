#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PORT 8081
#define MAX_USERS 10

typedef struct // Struct to pass arguments to the thread
{
    int newSocket;
    int *userMap;
} ThreadArgs;

typedef struct // Struct to represent a message
{
    /*
    message type / explanation
        -1      /  disconnect
        0       /  login request
        1       /  regular message
        2       /  registration request
    */
    int type;
    char body[1024];
    int to;   // -1 for server, user_id for specific user
    int from; // -1 for server, user_id for specific user
} Message;

void notifyClientsAndShutdown()
{
    Message disconnectMessage;
    disconnectMessage.type = -1; // -1 indicates a disconnect message
    int i;
    for (i = 0; i < MAX_USERS; i++)
    {

        send(i, &disconnectMessage, sizeof(disconnectMessage), 0);
        close(i);
    }

    close(0);
}

void disconnectClient(int newSocket, int *userMap)
{
    int userId = userMap[newSocket];
    printf("Client with user_id %d disconnected\n", userId);
    close(newSocket);
    userMap[newSocket] = -1; // Remove the user from the map
    pthread_exit(NULL);
}

int isUserRegistered(int userId)
{
    FILE *file = fopen("TerChatApp/users/user_list.txt", "r");
    if (file != NULL)
    {
        int id;
        while (fscanf(file, "%d", &id) != EOF)
        {
            if (id == userId)
            {
                fclose(file);
                return 1; // User is registered
            }
        }
        fclose(file);
    }
    else
    {
        printf("Error opening file\n");
    }
    return 0; // User is not registered
}

void handleLoginRequest(int newSocket, Message receivedMessage, int *userMap)
{
    printf("Login request received from client: %d userId: %d\n", newSocket, receivedMessage.from);
    if (isUserRegistered(receivedMessage.from))
    {
        printf("User is registered\n");
        // Continue with login process
        userMap[newSocket] = receivedMessage.from;
    }
    else
    {
        printf("User is not registered\n");
        // Reject login request and send registration request to the client
        Message registrationRequest;
        registrationRequest.type = 2; // 2 indicates a registration request
        registrationRequest.from = -1;
        send(newSocket, &registrationRequest, sizeof(registrationRequest), 0);
    }
}

void *handleClient(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int newSocket = threadArgs->newSocket;
    int *userMap = threadArgs->userMap;
    Message receivedMessage;

    while (1)
    {
        int valrec = recv(newSocket, &receivedMessage, sizeof(receivedMessage), 0);
        if (valrec <= 0) // Client disconnected
        {
            disconnectClient(newSocket, newSocket);
        }
        if (receivedMessage.type == -1) // disconnect request
        {
            disconnectClient(newSocket, newSocket);
        }
        else if (receivedMessage.type == 0) // login request
        {
            handleLoginRequest(newSocket, receivedMessage, newSocket);
        }
        else if (receivedMessage.type == 1)
        {
            printf("Message from client %d: %s\n", newSocket, receivedMessage.body);
        }
        else
        {
            printf("Client %d: %s\n", newSocket, receivedMessage.body);
        }
    }

    close(newSocket);
    pthread_exit(NULL);
}

int main()
{
    int userMap[MAX_USERS];          // Array to map socket numbers to user IDs
    mkdir("TerChatApp", 0777);       // Create the TerChatApp directory if it does not exist
    mkdir("TerChatApp/users", 0777); // Create the users directory if it does not exist

    int clients[MAX_USERS] = {0};
    pthread_t threads[MAX_USERS];
    int threadCount = 0;

    int serverSock = 0;
    struct sockaddr_in servAddr;
    int addrlen = sizeof(servAddr);

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0)
    {
        perror("Server Socket Failed");
        exit(EXIT_FAILURE);
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(PORT);

    // bind the socket
    if (bind(serverSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        perror("Binding Server socket err");
        exit(EXIT_FAILURE);
    }

    // listen the prot
    if (listen(serverSock, 5) < 0)
    {
        perror("server listen err");
        exit(EXIT_FAILURE);
    }

    atexit(notifyClientsAndShutdown);

    while (1)
    {
        int newClient = accept(serverSock, (struct sockaddr *)&servAddr, (socklen_t *)&addrlen);
        if (newClient < 0)
        {
            perror("Error! When server accepting new client");
            exit(EXIT_FAILURE);
        }

        printf("\nnew client connected with client id: %d\n", newClient);

        clients[threadCount] = newClient;
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        args->newSocket = newClient;
        args->userMap = userMap;

        int thread_create = pthread_create(&threads[threadCount], NULL, handleClient, (void *)&clients[threadCount]);
        if (thread_create < 0)
        {
            perror("thread create for client error");
            exit(EXIT_FAILURE);
        }

        threadCount++;
        if (threadCount >= MAX_USERS)
        {
            printf("too many clients.Abort new connections\n");
            close(newClient);
        }
    }

    return 0;
}
