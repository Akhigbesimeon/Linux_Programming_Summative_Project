#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Main function
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char library_id[20];

    // Initialize TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Configure server address 
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Client authentication
    printf("Enter your Library ID: ");
    fgets(library_id, sizeof(library_id), stdin);
    library_id[strcspn(library_id, "\n")] = 0; 

    // Transmit credentials
    send(sock, library_id, strlen(library_id), 0);
    
    // Process server authentication response
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);

    if (strcmp(buffer, "AUTH_FAIL") == 0) {
        printf("Authentication failed. Invalid Library ID.\n");
        close(sock);
        return 0; 
    } else if (strcmp(buffer, "AUTH_SUCCESS") == 0) {
        printf("Authentication successful!\n");
    }

    // Catalog reception 
    // Wait for the server to transmit the list of available/reserved books
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("%s\n", buffer);

    // Reservation request ---
    char book_id_str[10];
    printf("Enter the ID of the book you want to reserve: ");
    fgets(book_id_str, sizeof(book_id_str), stdin);
    book_id_str[strcspn(book_id_str, "\n")] = 0;

    // Transmit chosen Book ID
    send(sock, book_id_str, strlen(book_id_str), 0);

    // Server response & teardown
    // Await final status regarding the reservation attempt
    memset(buffer, 0, BUFFER_SIZE);
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Server response: %s\n", buffer);

    // Close the socket session on the client side
    close(sock);
    printf("\nSession closed. Goodbye, %s\n", library_id);

    return 0;
}
