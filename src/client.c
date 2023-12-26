#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024
#define MAX_USER_ID_LENGTH 3

typedef struct
{
    int type; // 0 for login request, 1 for regular message
    char body[1024];
    int to;   // -1 for server, user_id for specific user
    int from; // for the client, this is the user_id
} Message;

void validateUserId(char *userIdStr)
{
    char *endChar;
    long userId = strtol(userIdStr, &endChar, 10);

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
}

int main(int argc, char *argv[])
{
    validateUserId(argv[1]);

    char user_id[MAX_USER_ID_LENGTH];
    strcpy(user_id, argv[1]);

    int sock = 0;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    // socket file descriptor
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Error! when socket is creating");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    int connection = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connection < 0)
    {
        perror("Error! Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // Send user_id to server
    Message loginMessage;
    loginMessage.type = 0;
    strcpy(loginMessage.from, user_id);
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
        int valread = recv(sock, buffer, BUFFER_SIZE, 0);
        printf("Server: %s\n", buffer);
    }

    return 0;
}