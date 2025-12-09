


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "database.h"
#include "network.h"
#include "timer.h"
#include "../shared/config.h"

volatile sig_atomic_t server_running = 1;

void signal_handler(int sig) {
    (void)sig;
    printf("\n[INFO] Shutting down server...\n");
    server_running = 0;
}

int main() {
    printf("==============================================\n");
    printf("   ONLINE AUCTION SYSTEM - v2.0 MODULAR\n");
    printf("==============================================\n\n");
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize database
    if (db_init() < 0) {
        fprintf(stderr, "[ERROR] Failed to initialize database\n");
        return 1;
    }
    printf("[INFO] Database opened: %s\n", DATABASE_FILE);
    
    // Initialize client tracking
    init_clients();
    
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("[ERROR] Socket creation failed");
        db_close();
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[WARNING] setsockopt failed");
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[ERROR] Bind failed");
        close(server_socket);
        db_close();
        return 1;
    }
    
    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("[ERROR] Listen failed");
        close(server_socket);
        db_close();
        return 1;
    }
    
    printf("[INFO] Server listening on port %d\n", SERVER_PORT);
    printf("[INFO] Database: %s\n", DATABASE_FILE);
    
    // Start auction timer thread (30s warnings, auto-end)
    start_auction_timer();
    printf("[INFO] Auction timer started\n");
    
    printf("[INFO] Waiting for connections...\n\n");
    
    // Main server loop
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (!server_running) break;
            perror("[WARNING] Accept failed");
            continue;
        }
        
        printf("[INFO] Client connected: socket %d\n", client_socket);
        
        // Create thread for client
        pthread_t thread_id;
        int *sock = malloc(sizeof(int));
        if (sock == NULL) {
            perror("[ERROR] Memory allocation failed");
            close(client_socket);
            continue;
        }
        *sock = client_socket;
        
        if (pthread_create(&thread_id, NULL, handle_client, sock) != 0) {
            perror("[ERROR] Thread creation failed");
            close(client_socket);
            free(sock);
            continue;
        }
        
        pthread_detach(thread_id);
    }
    
    // Cleanup
    printf("[INFO] Closing server socket...\n");
    close(server_socket);
    
    printf("[INFO] Closing database...\n");
    db_close();
    
    printf("[INFO] Server shut down successfully!\n");
    
    return 0;
}