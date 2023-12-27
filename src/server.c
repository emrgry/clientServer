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
    int *clients;
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

typedef struct
{
    int userId;
    int count;
    struct Node *next;
} Node;

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

void sendConfirmationMessage(int newSocket, const char *message)
{
    Message confirmationMessage;
    confirmationMessage.type = 3; // Assuming 3 is the type for a registration confirmation
    strcpy(confirmationMessage.body, message);
    send(newSocket, &confirmationMessage, sizeof(confirmationMessage), 0);
}

int findSocketByUserId(int userId, int *clients)
{
    // Check if the user ID is within the range of the array
    if (userId < 0 || userId >= MAX_USERS)
    {
        printf("User ID out of range: %d\n", userId);
        return -1;
    }

    // Return the socket associated with the user ID
    return clients[userId];
}

void disconnectClient(int newSocket, int userId)
{
    // int userId = userMap[newSocket];
    printf("Client %d with userId %d  disconnected\n", newSocket, userId);
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

void handleLoginRequest(int newSocket, Message receivedMessage, int *clients)
{
    printf("Login request received from client: %d userId: %d\n", newSocket, receivedMessage.from);
    clients[receivedMessage.from] = newSocket;
    if (isUserRegistered(receivedMessage.from))
    {
        printf("User is registered\n");
        // Continue with login process
        // userMap[newSocket] = receivedMessage.from;
        sendConfirmationMessage(newSocket, "logged in");
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
    sendConfirmationMessage(newSocket, "registered");
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
        sendConfirmationMessage(sock, "Error occured in server");
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

    if (userCount == 0)
    {
        sendConfirmationMessage(sock, "Contact list is empty");
        return;
    }

    // Send the users array to the client
    int i;
    for (i = 0; i < userCount; i++)
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
        sendConfirmationMessage(sock, "Error occured in server");
        return;
    }

    int existingUserId;
    while (fscanf(file, "%d,%*[^\n]\n", &existingUserId) != EOF)
    {
        if (existingUserId == user.userId)
        {
            printf("User already exists in contact list\n");
            fclose(file);
            sendConfirmationMessage(sock, "User already exists in contact list");
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
    sendConfirmationMessage(sock, "User added to contact list");
}

void deleteUserFromFile(int sock, int userId, int userIdToDelete)
{
    FILE *file;
    char filename[50];
    sprintf(filename, "TerChatApp/users/%d/contact_list.txt", userId);

    // Open the file in read mode
    file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    // Read all the lines into a dynamic array
    char **lines = NULL;
    size_t len = 0;
    ssize_t read;
    int lineCount = 0;
    while ((read = getline(&lines[lineCount], &len, file)) != -1)
    {
        lineCount++;
        lines = realloc(lines, (lineCount + 1) * sizeof(char *));
        if (lines == NULL)
        {
            perror("Error reallocating memory");
            return;
        }
        lines[lineCount] = NULL;
    }

    // Close the file
    fclose(file);

    // Open the file again in write mode
    file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    // Write all the lines back to the file, except for the line that contains the user ID to be deleted
    int i;
    for (i = 0; i < lineCount; i++)
    {
        int userId;
        sscanf(lines[i], "%d", &userId);
        if (userId != userIdToDelete)
        {
            fprintf(file, "%s", lines[i]);
        }
        free(lines[i]);
    }

    // Close the file
    fclose(file);

    // Free the dynamic array
    free(lines);
    sendConfirmationMessage(sock, "User deleted from contact list");
}

void processMessage(int sock, int fromUserId, int toUserId, int recipientSocket, char *messageText)
{
    // Find the socket associated with the recipient user ID

    if (recipientSocket == -1)
    {
        printf("Recipient user ID not found: %d\n", toUserId);
        return;
    }

    // // Send the message to the recipient
    // Message msg;
    // msg.type = 7;                                         // Set the message type to 7 (send message)
    // msg.from = fromUserId;                                // Set the from field to the current userId
    // msg.to = toUserId;                                    // Set the to field to the userId of the recipient
    // strncpy(msg.body, messageText, sizeof(msg.body) - 1); // Copy the message text into the body field

    // if (send(recipientSocket, &msg, sizeof(msg), 0) == -1)
    // {
    //     perror("Error sending message");
    //     return;
    // }

    char filename[50];
    FILE *file;

    // Get the current date and time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char date[50];
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    // Write the message to the sender's messages file
    sprintf(filename, "TerChatApp/users/%d/messages.txt", fromUserId);
    file = fopen(filename, "a");
    if (file != NULL)
    {
        int readStatus = 0;
        fprintf(file, "%s, %d, %s, %d\n", date, toUserId, messageText, readStatus);
        fclose(file);
    }
    else
    {
        perror("Error opening sender's messages file");
    }

    // Write the message to the recipient's messages file
    sprintf(filename, "TerChatApp/users/%d/messages.txt", toUserId);
    file = fopen(filename, "a");
    if (file != NULL)
    {
        int readStatus = 0;
        fprintf(file, "%s, %d, %s, %d\n", date, fromUserId, messageText, readStatus);
        fclose(file);
    }
    else
    {
        perror("Error opening recipient's messages file");
    }
    sendConfirmationMessage(sock, "Message sent");
}

void countUnreadMessagesAndSend(int sock, int userId)
{
    printf("Counting unread messages for user %d\n", userId);
    char filename[50];
    sprintf(filename, "TerChatApp/users/%d/messages.txt", userId);

    FILE *file = fopen(filename, "r");
    if (file != NULL)
    {
        char line[256];
        int unreadCounts[MAX_USERS] = {0}; // This will hold the counts of unread messages for each user

        while (fgets(line, sizeof(line), file))
        {
            Message msg;

            // Parse the line
            sscanf(line, "%d, %[^,], %d, %d\n", &msg.type, msg.body, &msg.to, &msg.from);

            // Check if the message is unread (type 8)
            if (msg.type == 8)
            {
                unreadCounts[msg.from]++; // Increment the count for this user
            }
        }
        fclose(file);

        // Send the counts for each user to the client
        for (int i = 0; i < MAX_USERS; i++)
        {
            if (unreadCounts[i] > 0)
            {
                Message msg;
                msg.type = 8;
                sprintf(msg.body, "%d Unread message from user %d\n", unreadCounts[i], i);
                msg.to = userId;
                msg.from = -1; // from server

                if (send(sock, &msg, sizeof(msg), 0) == -1)
                {
                    perror("Error sending message");
                }
            }
        }
    }
    else
    {
        perror("Error opening messages file");
    }
}

void *handleClient(void *args)
{
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int newSocket = threadArgs->newSocket;
    int *clients = threadArgs->clients;
    Message receivedMessage;

    while (1)
    {
        int valrec = recv(newSocket, &receivedMessage, sizeof(receivedMessage), 0);
        if (valrec <= 0) // Client disconnected
        {
            printf("Client %d disconnected\n", newSocket);
            disconnectClient(newSocket, receivedMessage.from);
        }
        if (receivedMessage.type == -1) // disconnect request
        {
            disconnectClient(newSocket, receivedMessage.from);
        }
        else if (receivedMessage.type == 0) // login request
        {
            handleLoginRequest(newSocket, receivedMessage, clients);
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
        else if (receivedMessage.type == 6)
        {
            deleteUserFromFile(newSocket, receivedMessage.from, receivedMessage.to);
        }
        else if (receivedMessage.type == 7)
        {
            int recipientSocket = findSocketByUserId(receivedMessage.to, clients);
            processMessage(newSocket, receivedMessage.from, receivedMessage.to, recipientSocket, receivedMessage.body);
        }
        else if (receivedMessage.type == 8)
        {
            countUnreadMessagesAndSend(newSocket, receivedMessage.from);
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
    mkdir("TerChatApp", 0777);       // Create the TerChatApp directory if it does not exist
    mkdir("TerChatApp/users", 0777); // Create the users directory if it does not exist

    int clients[MAX_USERS] = {0};
    int sockets[MAX_USERS] = {0};
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
        args->clients = sockets;

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

    int i;
    for (i = 0; i < threadCount; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
