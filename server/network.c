#include "network.h"
#include "handlers.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

ClientSession g_clients[MAX_CLIENTS];
pthread_mutex_t g_client_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        g_clients[i].socket = -1;
        g_clients[i].user_id = -1;
        g_clients[i].current_room_id = -1;
        g_clients[i].is_active = 0;
    }
}

int add_client_session(int socket, int user_id) {
    pthread_mutex_lock(&g_client_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!g_clients[i].is_active) {
            g_clients[i].socket = socket;
            g_clients[i].user_id = user_id;
            g_clients[i].current_room_id = -1;
            g_clients[i].is_active = 1;
            
            pthread_mutex_unlock(&g_client_mutex);
            return i;
        }
    }
    
    pthread_mutex_unlock(&g_client_mutex);
    return -1;
}

void remove_client_session(int socket) {
    pthread_mutex_lock(&g_client_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].socket == socket && g_clients[i].is_active) {
            if (g_clients[i].user_id > 0) {
                db_leave_room(g_clients[i].user_id);
            }
            g_clients[i].is_active = 0;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_client_mutex);
}

void update_client_room(int user_id, int room_id) {
    pthread_mutex_lock(&g_client_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].user_id == user_id && g_clients[i].is_active) {
            g_clients[i].current_room_id = room_id;
            break;
        }
    }
    
    pthread_mutex_unlock(&g_client_mutex);
}

ClientSession* find_client_by_socket(int socket) {
    pthread_mutex_lock(&g_client_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].socket == socket && g_clients[i].is_active) {
            pthread_mutex_unlock(&g_client_mutex);
            return &g_clients[i];
        }
    }
    
    pthread_mutex_unlock(&g_client_mutex);
    return NULL;
}
void kick_existing_session(int user_id) {
    pthread_mutex_lock(&g_client_mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].is_active && g_clients[i].user_id == user_id) {
            char kick_msg[BUFFER_SIZE];
            sprintf(kick_msg, "KICKED|You have been logged in from another location\n");
            send(g_clients[i].socket, kick_msg, strlen(kick_msg), 0);
            
            close(g_clients[i].socket);
            g_clients[i].is_active = 0;
            
            printf("[INFO] Kicked user %d from old session\n", user_id);
            break;
        }
    }
    
    pthread_mutex_unlock(&g_client_mutex);
}

void* handle_client(void *arg) {
    int client_socket = *(int*)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    
    printf("[INFO] Client connected: socket %d\n", client_socket);
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes <= 0) {
            break;
        }
        
        buffer[bytes] = '\0';
        
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        handle_request(client_socket, buffer);
    }
    
    printf("[INFO] Client disconnected: socket %d\n", client_socket);
    remove_client_session(client_socket);
    close(client_socket);
    
    return NULL;
}
