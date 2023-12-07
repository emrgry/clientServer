#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8081

int main()
{
    
    int clients[5] = {0};

    int server_sock = 0;
    struct sockaddr_in serv_addr;
    

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("Server Socket Failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    //bind the socket 
    if (bind(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
    {
        perror("Binding Server socket err");
        exit(EXIT_FAILURE);
    }

    //listen the prot
    if (listen(server_sock, 5) < 0)
    {
        perror("server listen err");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        int new_client = accept(server_sock, (struct sockaddr*)&serv_addr, (size_t)sizeof(serv_addr));
        if (new_client < 0)
        {
            perror("Accepting new client error");
            exit(EXIT_FAILURE);
        }

        printf("\nnew client connected\n");


    }


    return 0;
}
