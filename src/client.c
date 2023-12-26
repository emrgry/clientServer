#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024
#define MAX_USER_NAME 20

int main(int argc, char *argv[])
{
    char user_name[MAX_USER_NAME];
    strcpy(user_name, argv[1]);

    printf("User name: %s\n", user_name);
    printf("First argument: %s\n", argv[1]);
    int sock = 0;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    // socket file descriptor
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket Creating failed");
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
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));

        // Read message from user
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Send message to server
        send(sock, buffer, strlen(buffer), 0);
        printf("Message sent to server\n");

        // Receive message from server
        int valread = recv(sock, buffer, BUFFER_SIZE, 0);
        printf("Server: %s\n", buffer);
    }

    return 0;
}