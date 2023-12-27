#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define BUFFER_SIZE 1024
#define REGISTRATION_BUFFER_SIZE 16
#define MAX_USER_ID_LENGTH 3
#define MAX_USERS 10

typedef struct
{
    /*
    message type / explanation
        -1       /  disconnect
        0        /  login request
        1        /  server message
        2        /  registration request
        3        /  confirmation message
        4        /  list contacts
        5        /  add user
        6        /  delete user
        7        /  send message
        8        /  check message
    */
    int type; // -1 for disconnect, 0 for login request, 1 for regular message
    char body[1024];
    int to;   // -1 for server, user_id for specific user
    int from; // for the client, this is the user_id
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

void removeNewline(char *string)
{
    int length = strlen(string);
    if (string[length - 1] == '\n')
    {
        string[length - 1] = '\0';
    }
}

void registerUser(int sock, int userId)
{
    printf("Please register for using this app\n");

    char *username = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));
    char *phoneNumber = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));
    char *name = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));
    char *surname = malloc(REGISTRATION_BUFFER_SIZE * sizeof(char));

    printf("Enter your username: ");
    fgets(username, REGISTRATION_BUFFER_SIZE, stdin);
    removeNewline(username);

    printf("Enter your phone number: ");
    fgets(phoneNumber, REGISTRATION_BUFFER_SIZE, stdin);
    removeNewline(phoneNumber);

    printf("Enter your name: ");
    fgets(name, REGISTRATION_BUFFER_SIZE, stdin);
    removeNewline(name);

    printf("Enter your surname: ");
    fgets(surname, REGISTRATION_BUFFER_SIZE, stdin);
    removeNewline(surname);

    // Create a Message for the user's information
    Message userInfo;
    userInfo.type = 2;
    userInfo.from = userId;
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

void listContacts(int sock, int userId)
{
    Message msg;
    msg.type = 4; // Assuming 1 is the type for "list contacts" request
    msg.from = userId;
    msg.to = -1;
    strcpy(msg.body, "List contacts request");

    if (send(sock, &msg, sizeof(msg), 0) == -1)
    {
        perror("Error sending list contacts request");
    }
}

User CreateUser()
{
    User newUser;
    printf("Enter user ID: ");
    scanf("%d", &newUser.userId);
    getchar(); // To consume the newline character after the number

    // printf("Enter username: ");
    // fgets(newUser.username, sizeof(newUser.username), stdin);
    // newUser.username[strcspn(newUser.username, "\n")] = 0; // Remove the newline character

    printf("Enter phone number: ");
    fgets(newUser.phoneNumber, sizeof(newUser.phoneNumber), stdin);
    newUser.phoneNumber[strcspn(newUser.phoneNumber, "\n")] = 0; // Remove the newline character

    printf("Enter name: ");
    fgets(newUser.name, sizeof(newUser.name), stdin);
    newUser.name[strcspn(newUser.name, "\n")] = 0; // Remove the newline character

    printf("Enter surname: ");
    fgets(newUser.surname, sizeof(newUser.surname), stdin);
    newUser.surname[strcspn(newUser.surname, "\n")] = 0; // Remove the newline character
    return newUser;
}

void addUser(int sock, int userId, User user)
{
    Message msg;
    msg.type = 5;
    msg.from = userId;                      // Set the message type to 5 (add user)
    memcpy(&msg.body, &user, sizeof(User)); // Copy the user struct into the message body

    if (send(sock, &msg, sizeof(msg), 0) == -1)
    {
        perror("Error sending user");
    }
}

// list contacts
void processUserList(Message receivedMessage, int sock, int userId)
{
    User *users = malloc(MAX_USERS * sizeof(User));
    if (users == NULL)
    {
        perror("Error allocating memory for users");
        return;
    }

    int userCount = receivedMessage.to; // sizeof(receivedMessage.body) / sizeof(User);
    // memcpy(users, &receivedMessage.body, sizeof(receivedMessage.body)); // Copy the user structs from the message body
    memcpy(users, &receivedMessage.body, userCount * sizeof(User));
    printf("User ID, Name, Surname, Phone Number\n");
    int i;
    for (i = 0; i < userCount; i++)
    {
        printf("%d, %s, %s, %s\n", users[i].userId, users[i].name, users[i].surname, users[i].phoneNumber);
    }

    free(users);
}

void deleteUser(int sock, int userId)
{
    int deleteUserId;
    printf("Enter the ID of the user to be deleted: ");
    scanf("%d", &deleteUserId);

    Message msg;
    msg.type = 6;          // Set the message type to 6 (delete user)
    msg.from = userId;     // Set the from field to the current userId
    msg.to = deleteUserId; // Set the to field to the userId of the user to be deleted

    if (send(sock, &msg, sizeof(msg), 0) == -1)
    {
        perror("Error sending delete user request");
    }
}

void sendMessage(int sock, int userId)
{
    int recipientUserId;
    printf("Enter the ID of the user to send the message to: ");
    scanf("%d", &recipientUserId);

    // Clear the input buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }

    char messageText[256];
    printf("Enter your message: ");
    fgets(messageText, sizeof(messageText), stdin);

    // Remove trailing newline
    messageText[strcspn(messageText, "\n")] = 0;

    Message msg;
    msg.type = 7;                                         // Set the message type to 7 (send message)
    msg.from = userId;                                    // Set the from field to the current userId
    msg.to = recipientUserId;                             // Set the to field to the userId of the recipient
    strncpy(msg.body, messageText, sizeof(msg.body) - 1); // Copy the message text into the body field

    if (send(sock, &msg, sizeof(msg), 0) == -1)
    {
        perror("Error sending message");
    }
}

void HandleMenu(int sock, int userId)
{
    printf("<--------------------------->\n");
    printf("Please type your choice:\n");
    printf("1 - List contacts\n");
    printf("2 - Add user\n");
    printf("3 - Delete user\n");
    printf("4 - Send message\n");
    printf("5 - Check message\n");
    printf("6 - Disconnect\n");

    int choice;
    scanf("%d", &choice);
    getchar(); // To consume the newline character after the number
    printf("<--------------------------->\n");
    switch (choice)
    {
    case 1:
        // Call function to list contacts
        listContacts(sock, userId);
        break;
    case 2:
        // Call function to add user
        addUser(sock, userId, CreateUser());
        break;
    case 3:
        // Call function to delete user
        deleteUser(sock, userId);
        break;
    case 4:
        // Call function to send message
        sendMessage(sock, userId);
        break;
    case 5:
        // Call function to check message
        break;
    case 6:
        disconnect(sock, userId);
        break;
    default:
        printf("Invalid choice. Please try again.\n");
        return;
    }
    return;
}

int main(int argc, char *argv[])
{
    int userId = validateUserId(argv[1]);

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
    loginMessage.from = userId;
    loginMessage.to = -1; // this message will processed by server
    send(sock, &loginMessage, sizeof(loginMessage), 0);
    printf("Login request sent to server\n");

    while (1)
    {

        // Receive message from server
        Message receivedMessage;
        // int valrec = recv(sock, &receivedMessage, BUFFER_SIZE, 0);
        int valrec = recv(sock, &receivedMessage, sizeof(Message), 0);
        if (valrec <= 0 || receivedMessage.type == -1) // disconnect request or connection closed
        {
            printf("Disconnect request received from server or connection closed\n");
            close(sock);
            exit(0);
        }
        else if (receivedMessage.type == 2) // registration request
        {
            registerUser(sock, userId);
        }
        else if (receivedMessage.type == 3)
        {
            // confirmation message
            printf("<!!!!!!!!!!!!!!!!!!!!!!!!!!!>\n");
            printf("Server notification! %s\n", receivedMessage.body);
            printf("<!!!!!!!!!!!!!!!!!!!!!!!!!!!>\n");
            HandleMenu(sock, userId);
        }
        else if (receivedMessage.type == 4)
        {
            // list contacts
            processUserList(receivedMessage, sock, userId);
            HandleMenu(sock, userId);
        }
        else if (receivedMessage.type == 7)
        {
            printf("Message received from %d: %s\n", receivedMessage.from, receivedMessage.body);
            HandleMenu(sock, userId);
        }
        else
        {
            printf("Server %d: %s, message type %d\n", sock, receivedMessage.body, receivedMessage.type);
        }

        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));
    }

    return 0;
}