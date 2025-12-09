
#include "network.h"
#include "../shared/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int connect_to_server(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return -1;
    }
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }
    
    return sock;
}

void disconnect_from_server(int socket) {
    if (socket >= 0) {
        close(socket);
    }
}

int send_request(int socket, const char *request) {
    int len = strlen(request);
    int sent = send(socket, request, len, 0);
    return sent;
}

int receive_response(int socket, char *buffer, int buffer_size) {
    memset(buffer, 0, buffer_size);
    
    int attempts = 0;
    int max_attempts = 30; // 3 seconds max (30 * 100ms)
    
    while (attempts < max_attempts) {
        // Peek to check message type
        int n = recv(socket, buffer, buffer_size - 1, MSG_PEEK | MSG_DONTWAIT);
        
        if (n > 0) {
            buffer[n] = '\0';
            
            
            if (strncmp(buffer, "NOTIF_", 6) == 0) {
                // Notification - wait for notification thread to consume
                usleep(20000); // 20ms
                attempts++;
                continue;
            }
            
            // KICKED is special - let notification thread handle
            if (strncmp(buffer, "KICKED|", 7) == 0) {
                usleep(20000);
                attempts++;
                continue;
            }
            
            // This is a real response - consume it NOW
            memset(buffer, 0, buffer_size);
            n = recv(socket, buffer, buffer_size - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                
                // Remove trailing newline
                char *newline = strchr(buffer, '\n');
                if (newline) *newline = '\0';
                
                return n;
            }
        } else if (n == 0) {
            // Connection closed
            return 0;
        } else {
            // No data yet - wait briefly
            usleep(100000); // 100ms
            attempts++;
        }
    }
    
    // Timeout - try one blocking recv
    int received = recv(socket, buffer, buffer_size - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
    }
    return received;
}   