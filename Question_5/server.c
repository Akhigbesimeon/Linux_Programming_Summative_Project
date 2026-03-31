/* 
Documentation
-------------- 
1. Concurrency model (threads):
The main loop operates as a dispatcher. It blocks on accept() waiting for 
incoming connections. When a client connects, it allocates a specific socket 
for that connection and spawns a new POSIX thread (pthread). 
pthread_detach() is called immediately to detach the thread state from the 
main process, allowing the OS to automatically reclaim thread resources upon 
completion without needing the main thread to call pthread_join().

2. Authentication mechanism:
The server waits for the client to send their ID. Once received, it locks 
the mutex and verifies the ID against the 'valid_users' database. If matched, 
the user's session is flagged as active, preventing unauthorized routing 
or access to the book catalog. If not matched, it sends an error and drops.

3. Message protocol design:
The protocol relies on simple text-based messaging over TCP to ensure 
cross-platform compatibility without endianness issues.
* Client - Server: [LIBRARY_ID] (e.g., "LIB101") sent immediately.
* Server - Client: "AUTH_SUCCESS" or "AUTH_FAIL".
* Server - Client: Multiline string containing the catalog and statuses.
* Client - Server: [BOOK_ID] (e.g., "3") representing reservation choice.
* Server - Client: Final status ("Reserved successfully.", etc.).

4. Shared resource protection:
The 'lib_mutex' protects the shared 'library' array and 'valid_users' array.
Any thread attempting to read or modify book statuses or user login states 
must acquire this lock first. This serializes access, entirely preventing 
race conditions (e.g., two users reserving the same book simultaneously).

5. Session lifecycle handling:
Sessions are bound directly to the thread. If a client disconnects, finishes, 
or drops off, the thread handles the cleanup. It safely updates the active 
user state back to 0, closes the socket descriptor to prevent FD leaks, 
and calls pthread_exit() to terminate the session independently 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define NUM_BOOKS 5
#define NUM_USERS 3

typedef struct {
    int id;
    char title[100];
    int is_reserved;
} Book;

typedef struct {
    char lib_id[20];
    int is_active;
} User;

// Global shared resources
Book library[NUM_BOOKS] = {
    {1, "A Tale of Two Cities", 0},
    {2, "Pride & Prejudice", 0},
    {3, "The Chronicles of Narnia", 0},
    {4, "Harry Potter", 0},
    {5, "The Godfather", 0}
};

User valid_users[NUM_USERS] = {
    {"LIB101", 0},
    {"LIB102", 0},
    {"LIB103", 0}
};

// Mutex for shared resource protection
pthread_mutex_t lib_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *client_socket) {
    // Extract socket descriptor and free allocated memory
    int sock = *(int*)client_socket;
    free(client_socket);
    
    char buffer[BUFFER_SIZE];
    char current_user_id[20] = "";
    int authenticated = 0;
    int user_index = -1;

    // Authentication 
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
        close(sock);
        pthread_exit(NULL);
    }

    // Sanitize incoming ID
    buffer[strcspn(buffer, "\n")] = 0;
    strncpy(current_user_id, buffer, sizeof(current_user_id) - 1);

    // Check user credentials
    pthread_mutex_lock(&lib_mutex);
    for (int i = 0; i < NUM_USERS; i++) {
        if (strcmp(valid_users[i].lib_id, current_user_id) == 0) {
            authenticated = 1;
            valid_users[i].is_active = 1; // Mark session active
            user_index = i;
            printf("[SERVER] User authenticated: %s\n", current_user_id);
            break;
        }
    }
    pthread_mutex_unlock(&lib_mutex);

    // Handle authentication result
    if (!authenticated) {
        printf("[SERVER] Failed authentication attempt for ID: %s\n", current_user_id);
        send(sock, "AUTH_FAIL", strlen("AUTH_FAIL"), 0);
        close(sock);
        pthread_exit(NULL);
    } else {
        send(sock, "AUTH_SUCCESS", strlen("AUTH_SUCCESS"), 0);
        usleep(100000);
    }

    // Send book list
    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "\n-- Available Books --\n");
    
    // Read book statuses
    pthread_mutex_lock(&lib_mutex);
    for (int i = 0; i < NUM_BOOKS; i++) {
        char line[150];
        snprintf(line, sizeof(line), "[%d] %s - %s\n", 
                 library[i].id, 
                 library[i].title, 
                 library[i].is_reserved ? "RESERVED" : "AVAILABLE");
        strcat(buffer, line);
    }
    pthread_mutex_unlock(&lib_mutex);
    
    send(sock, buffer, strlen(buffer), 0);

    // Handle reservation
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sock, buffer, BUFFER_SIZE, 0) > 0) {
        int requested_book_id = atoi(buffer);
        int success = 0;
        char response[BUFFER_SIZE];

        // Check and update specific book reservation status
        pthread_mutex_lock(&lib_mutex);
        int book_found = 0;
        for (int i = 0; i < NUM_BOOKS; i++) {
            if (library[i].id == requested_book_id) {
                book_found = 1;
                if (library[i].is_reserved == 0) {
                    library[i].is_reserved = 1; // Secure the reservation
                    success = 1;
                    printf("[SERVER] Book '%s' reserved by %s\n", library[i].title, current_user_id);
                }
                break;
            }
        }
        pthread_mutex_unlock(&lib_mutex);

        // Formulate and send response back to client
        if (success) {
            snprintf(response, sizeof(response), "Reserved successfully.");
        } else if (!book_found) {
            snprintf(response, sizeof(response), "Invalid Book ID.");
        } else {
            snprintf(response, sizeof(response), "Already reserved.");
        }
        send(sock, response, strlen(response), 0);
    }

    // Session Cleanup 
    // Update user state upon disconnect
    pthread_mutex_lock(&lib_mutex);
    if (user_index != -1) {
        valid_users[user_index].is_active = 0;
        printf("[SERVER] User disconnected: %s\n", current_user_id);
    }
    pthread_mutex_unlock(&lib_mutex);

    // Close socket and terminate thread safely
    close(sock);
    pthread_exit(NULL);
}

// Main function
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Setup TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to reuse address and port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port 8080
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Digital Library Server started on port %d...\n", PORT);

    // Main dispatcher loop
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Allocate memory for the socket descriptor to pass to the thread safely
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        // Spawn a new detached thread for the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)client_sock) < 0) {
            perror("Could not create thread");
            free(client_sock);
        }
        pthread_detach(thread_id); 
    }

    // Cleanup resources
    pthread_mutex_destroy(&lib_mutex);
    return 0;
}
