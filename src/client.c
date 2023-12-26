#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024
#define REGISTRATION_BUFFER_SIZE 64
#define MAX_USER_ID_LENGTH 3

typedef struct
{
    /*
    message type / explanation
        -1      /  disconnect
        0       /  login request
        1       /  regular message
        2       /  registration request
    */
    int type; // -1 for disconnect, 0 for login request, 1 for regular message
    char body[1024];
    int to;   // -1 for server, user_id for specific user
    int from; // for the client, this is the user_id
} Message;

int validateUserId(char *userIdStr)
{
    char *endChar;
    int userId = (int)strtol(userIdStr, &endChar, 10);

    if (*endChar != '\0' || endChar == userIdStr)
    {
        printf("Error: Argument is not a valid integer\n");
        exit(EXIT_FAILURE);
    }
    else if (userId < 0 || userId > 999)
    {
        printf("Error: Argument is not in range [0, 999]\n");
        exit(EXIT_FAILURE);
    }
    return userId;
}

// Send a disconnect message to the server
void disconnect(int sock, int user_id)
{
    Message disconnectMessage;
    disconnectMessage.type = -1; // -1 indicates a disconnect message
    disconnectMessage.from = user_id;
    send(sock, &disconnectMessage, sizeof(disconnectMessage), 0);
    printf("Disconnect request sent to server\n");
}

void registerUser(int sock)
{
    printf("Please register for using this app\n");

    char *username = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));
    char *phoneNumber = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));
    char *name = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));
    char *surname = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));

    printf("Enter your username: ");
    fgets(username, REGISTRATION_BUFFER_SIZE, stdin);

    printf("Enter your phone number: ");
    fgets(phoneNumber, REGISTRATION_BUFFER_SIZE, stdin);

    printf("Enter your name: ");
    fgets(name, REGISTRATION_BUFFER_SIZE, stdin);

    printf("Enter your surname: ");
    fgets(surname, REGISTRATION_BUFFER_SIZE, stdin);

    // Create a Message for the user's information
    Message userInfo;
    userInfo.type = 2;
    sprintf(userInfo.body, "%s,%s,%s,%s", username, phoneNumber, name, surname);

    // Send user info to server
    send(sock, &userInfo, sizeof(userInfo), 0);
    printf("User info sent to server\n");

    // Free the allocated memory
    free(username);
    free(phoneNumber);
    free(name);
    free(surname);
}

int main(int argc, char *argv[])
{
    int user_id = validateUserId(argv[1]);

    int sock = 0;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE] = {0};

    // socket file descriptor
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Error! when socket is creating");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    int connection = connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (connection < 0)
    {
        perror("Error! Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // Send user_id to server
    Message loginMessage;
    loginMessage.type = 0;
    loginMessage.from = user_id;
    loginMessage.to = -1; // this message will processed by server
    send(sock, &loginMessage, sizeof(loginMessage), 0);
    printf("Login request sent to server\n");

    while (1)
    {
        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));

        // Read message from user
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Create a Message for the user's message
        Message userMessage;
        userMessage.type = 1;
        strcpy(userMessage.body, buffer);
        userMessage.to = -1; // Sending to server

        // Send message to server
        send(sock, &userMessage, sizeof(userMessage), 0);
        printf("Message sent to server\n");

        // Receive message from server
        Message receivedMessage;
        int valrec = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valrec <= 0 || receivedMessage.type == -1) // disconnect request or connection closed
        {
            printf("Disconnect request received from server or connection closed\n");
            close(sock);
            exit(0);
        }
        else if (receivedMessage.type == 2) // registration request
        {
            registerUser(sock);
        }

        printf("Server: %s\n", buffer);
    }

    return 0;
}