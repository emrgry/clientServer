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
#define REGISTRATION_BUFFER_SIZE 16

typedef struct // Struct to pass arguments to the thread
{
    int newSocket;
    // int *userMap;
} ThreadArgs;

typedef struct // Struct to represent a message
{
    /*
    message type / explanation
        -1      /  disconnect
        0       /  login request
        1       /  regular message
        2       /  registration request
        3       /  confirmation message
    */
    int type;
    char body[1024];
    int to;   // -1 for server, user_id for specific user
    int from; // -1 for server, user_id for specific user
} Message;

typedef struct
{
    /* data */
    int userId;
    char username[REGISTRATION_BUFFER_SIZE];
    char phoneNumber[REGISTRATION_BUFFER_SIZE];
    char name[REGISTRATION_BUFFER_SIZE];
    char surname[REGISTRATION_BUFFER_SIZE];
} User;

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

void disconnectClient(int newSocket)
{
    // int userId = userMap[newSocket];
    printf("Client with  %d disconnected\n", newSocket);
    close(newSocket);
    // userMap[newSocket] = -1; // Remove the user from the map
    pthread_exit(NULL);
}

int isUserRegistered(int userId)
{
    FILE *file = fopen("TerChatApp/users/user_list.txt", "r");
    if (file != NULL)
    {
        int id;
        char username[1024], phoneNumber[1024], name[1024], surname[1024];
        while (fscanf(file, "%d,%[^,],%[^,],%[^,],%[^\n]\n", &id, username, phoneNumber, name, surname) != EOF)
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

void handleLoginRequest(int newSocket, Message receivedMessage)
{
    printf("Login request received from client: %d userId: %d\n", newSocket, receivedMessage.from);
    if (isUserRegistered(receivedMessage.from))
    {
        printf("User is registered\n");
        // Continue with login process
        // userMap[newSocket] = receivedMessage.from;
        Message confirmationMessage;
        confirmationMessage.type = 3; // Assuming 3 is the type for a registration confirmation
        strcpy(confirmationMessage.body, "loggedin");
        send(newSocket, &confirmationMessage, sizeof(confirmationMessage), 0);
    }
    else
    {
        printf("User is not registered\n");
        // Reject login request and send registration request to the client
        Message registrationRequest;
        registrationRequest.type = 2; // 2 indicates a registration request
        send(newSocket, &registrationRequest, sizeof(registrationRequest), 0);
    }
}

void handleRegistrationRequest(int newSocket, Message receivedMessage)
{
    printf("Registration request received from client: %d userId: %d\n", newSocket, receivedMessage.from);

    // Extract user's information from receivedMessage.body
    char *username = strtok(receivedMessage.body, ",");
    char *phoneNumber = strtok(NULL, ",");
    char *name = strtok(NULL, ",");
    char *surname = strtok(NULL, ",");

    // Print received information
    printf("Received registration request:\n");
    printf("Username: %s\n", username);
    printf("Phone number: %s\n", phoneNumber);
    printf("Name: %s\n", name);
    printf("Surname: %s\n", surname);

    // Open the file for appending
    FILE *file = fopen("TerChatApp/users/user_list.txt", "a");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return;
    }
    // Write the user's information to the file
    fprintf(file, "%d,%s,%s,%s,%s\n", receivedMessage.from, username, phoneNumber, name, surname);
    fclose(file);

    // Create a directory for the user
    char *dirPath = malloc((strlen("TerChatApp/users/") + sizeof(receivedMessage.from) + 1) * sizeof(char));
    sprintf(dirPath, "TerChatApp/users/%d", receivedMessage.from);
    if (mkdir(dirPath, 0777) == -1)
    {
        printf("Error creating directory\n");
        return;
    }

    // Create a file for the user's contact list
    char *filePath = malloc((strlen(dirPath) + strlen("/contact_list.txt") + 1) * sizeof(char));
    sprintf(filePath, "%s/contact_list.txt", dirPath);
    file = fopen(filePath, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        free(dirPath);
        free(filePath);
        return;
    }
    fclose(file);
    free(filePath);

    // Create a file for the user's messages
    filePath = malloc((strlen(dirPath) + strlen("/messages.txt") + 1) * sizeof(char));
    sprintf(filePath, "%s/messages.txt", dirPath);
    file = fopen(filePath, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        free(dirPath);
        free(filePath);
        return;
    }
    fclose(file);

    free(filePath);
    free(dirPath);

    // Send a confirmation message back to the client
    Message confirmationMessage;
    confirmationMessage.from = -1; // -1 indicates a message from the server
    confirmationMessage.to = receivedMessage.from;
    confirmationMessage.type = 3; // Assuming 3 is the type for a registration confirmation
    strcpy(confirmationMessage.body, "registered");
    send(newSocket, &confirmationMessage, sizeof(confirmationMessage), 0);
}

void sendContactList(int sock, int userId)
{
    User users[MAX_USERS];
    int userCount = 0;

    char filePath[100];
    sprintf(filePath, "TerChatApp/users/%d/contact_list.txt", userId);
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("Error opening contact list");
        return;
    }

    // Read the contact list into the users array
    while (fscanf(file, "%d,%[^,],%[^,],%[^\n]\n", &users[userCount].userId, users[userCount].name, users[userCount].surname, users[userCount].phoneNumber) != EOF)
    {
        userCount++;
        if (userCount >= MAX_USERS)
        {
            break;
        }
    }

    fclose(file);

    // Send the users array to the client
    for (int i = 0; i < userCount; i++)
    {
        Message msg;
        msg.type = 4; // Set the message type to 4
        msg.from = -1;
        msg.to = userCount;                         // Set the message to to the number of users
        memcpy(&msg.body, &users[i], sizeof(User)); // Copy the user struct into the message body
        if (send(sock, &msg, sizeof(msg), 0) == -1)
        {
            perror("Error sending user");
            return;
        }
    }
}

void addUserToContactList(int sock, int userId, User user)
{
    char filePath[100];
    sprintf(filePath, "TerChatApp/users/%d/contact_list.txt", userId);
    printf("Adding user to contact list: %s\n", filePath);

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("Error opening contact list");
        return;
    }

    User existingUser;
    while (fscanf(file, "%d,%[^,],%[^,],%[^\n]\n", &existingUser.userId, existingUser.name, existingUser.surname, existingUser.phoneNumber) != EOF)
    {
        if (existingUser.userId == user.userId)
        {
            printf("User already exists in contact list\n");
            fclose(file);
            return;
        }
    }

    fclose(file);

    file = fopen(filePath, "a");
    if (file == NULL)
    {
        perror("Error opening contact list");
        return;
    }

    fprintf(file, "%d,%s,%s,%s\n", user.userId, user.name, user.surname, user.phoneNumber);

    fclose(file);
}

void *handleClient(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int newSocket = threadArgs->newSocket;
    // int *userMap = threadArgs->userMap;
    Message receivedMessage;

    while (1)
    {
        int valrec = recv(newSocket, &receivedMessage, sizeof(receivedMessage), 0);
        if (valrec <= 0) // Client disconnected
        {
            printf("Client %d disconnected\n", newSocket);
            disconnectClient(newSocket);
        }
        if (receivedMessage.type == -1) // disconnect request
        {
            disconnectClient(newSocket);
        }
        else if (receivedMessage.type == 0) // login request
        {
            handleLoginRequest(newSocket, receivedMessage);
        }

        else if (receivedMessage.type == 1)
        {
            printf("Message from client %d: %s\n", newSocket, receivedMessage.body);
        }
        else if (receivedMessage.type == 2)
        {
            handleRegistrationRequest(newSocket, receivedMessage);
        }
        else if (receivedMessage.type == 4)
        {
            sendContactList(newSocket, receivedMessage.from);
        }
        else if (receivedMessage.type == 5)
        {
            User userToAdd;
            memcpy(&userToAdd, receivedMessage.body, sizeof(User));
            addUserToContactList(newSocket, receivedMessage.from, userToAdd);
        }
        else
        {
            printf("Client %d: %s\n", newSocket, receivedMessage.body);
        }
    }

    close(newSocket);
    free(args);
    pthread_exit(NULL);
}

int main()
{
    // I got error so I disable disconnect feature
    // int userMap[MAX_USERS]; // Array to map socket numbers to user IDs
    // int *userMap = malloc(MAX_USERS * sizeof(int));
    // if (userMap == NULL)
    // {
    //     fprintf(stderr, "Failed to allocate memory for userMap\n");
    //     exit(1);
    // }
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
        // args->userMap = userMap;

        int thread_create = pthread_create(&threads[threadCount], NULL, handleClient, (void *)args);
        if (thread_create < 0)
        {
            perror("thread create for client error");
            exit(EXIT_FAILURE);
        }

        // Detach the thread
        pthread_detach(threads[threadCount]);

        threadCount++;
        if (threadCount >= MAX_USERS)
        {
            printf("too many clients.Abort new connections\n");
            close(newClient);
        }
    }

    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
