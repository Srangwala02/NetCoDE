#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Send username
    char username[50];
    printf("Enter username :");
    fgets(username, sizeof(username), stdin);

    // Send the message to the server
    if (send(client_socket, username, strlen(username), 0) == -1) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    // Prepare the message
    char docname[1024];
    printf("Enter document name: ");
    fgets(docname, sizeof(docname), stdin);

    // Send the message to the server
    if (send(client_socket, docname, strlen(docname), 0) == -1) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }

    // Close the socket
    close(client_socket);

    return 0;
}
