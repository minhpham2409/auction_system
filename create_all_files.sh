#!/bin/bash

# Create handlers.c
cat > server/handlers.c << 'EOF'
#include "handlers.h"
#include "database.h"
#include "network.h"
#include "../shared/config.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void handle_register(int client_socket, char *data) {
    char username[50], password[100];
    double balance = 50000000.0;
    
    sscanf(data, "%[^|]|%s", username, password);
    
    int user_id = db_create_user(username, password, balance);
    
    char response[BUFFER_SIZE];
    if (user_id > 0) {
        sprintf(response, "REGISTER_SUCCESS|%d|%s\n", user_id, username);
        printf("[INFO] New user registered: %s (ID: %d)\n", username, user_id);
    } else {
        sprintf(response, "REGISTER_FAIL|Username already exists\n");
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_login(int client_socket, char *data) {
    char username[50], password[100];
    sscanf(data, "%[^|]|%s", username, password);
    
    int user_id = db_authenticate_user(username, password);
    
    char response[BUFFER_SIZE];
    if (user_id > 0) {
        User user;
        db_get_user(user_id, &user);
        
        sprintf(response, "LOGIN_SUCCESS|%d|%s|%.2f\n", 
                user_id, username, user.balance);
        
        add_client_session(client_socket, user_id);
        printf("[INFO] User logged in: %s (ID: %d)\n", username, user_id);
    } else {
        sprintf(response, "LOGIN_FAIL|Invalid username or password\n");
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_create_room(int client_socket, char *data) {
    int creator_id, max_participants, duration;
    char name[100], desc[200];
    
    sscanf(data, "%d|%[^|]|%[^|]|%d|%d",
           &creator_id, name, desc, &max_participants, &duration);
    
    int current_room = db_get_user_room(creator_id);
    if (current_room > 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "CREATE_ROOM_FAIL|You must leave your current room first\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    int room_id = db_create_room(creator_id, name, desc, max_participants, duration);
    
    char response[BUFFER_SIZE];
    if (room_id > 0) {
        db_join_room(creator_id, room_id);
        update_client_room(creator_id, room_id);
        
        sprintf(response, "CREATE_ROOM_SUCCESS|%d|%s\n", room_id, name);
        printf("[INFO] Room created: %s (ID: %d)\n", name, room_id);
    } else {
        sprintf(response, "CREATE_ROOM_FAIL|Failed to create room\n");
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_list_rooms(int client_socket) {
    AuctionRoom rooms[MAX_ROOMS];
    int count = db_get_all_rooms(rooms, MAX_ROOMS);
    
    char response[BUFFER_SIZE * 4] = "ROOM_LIST|";
    char temp[256];
    
    for (int i = 0; i < count; i++) {
        User creator;
        db_get_user(rooms[i].created_by, &creator);
        
        sprintf(temp, "%d;%s;%s;%d;%d|",
                rooms[i].room_id,
                rooms[i].room_name,
                creator.username,
                rooms[i].current_participants,
                rooms[i].max_participants);
        strcat(response, temp);
    }
    
    strcat(response, "\n");
    send(client_socket, response, strlen(response), 0);
}

void handle_join_room(int client_socket, char *data) {
    int user_id, room_id;
    sscanf(data, "%d|%d", &user_id, &room_id);
    
    int result = db_join_room(user_id, room_id);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        AuctionRoom room;
        db_get_room(room_id, &room);
        
        sprintf(response, "JOIN_ROOM_SUCCESS|%d|%s\n", room_id, room.room_name);
        update_client_room(user_id, room_id);
        
        printf("[INFO] User %d joined room %d\n", user_id, room_id);
    } else {
        const char *error_msg;
        switch(result) {
            case -1: error_msg = "Room not found"; break;
            case -3: error_msg = "Room is full"; break;
            case -4: error_msg = "Already in another room"; break;
            default: error_msg = "Unknown error"; break;
        }
        sprintf(response, "JOIN_ROOM_FAIL|%s\n", error_msg);
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_leave_room(int client_socket, char *data) {
    int user_id;
    sscanf(data, "%d", &user_id);
    
    int result = db_leave_room(user_id);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        sprintf(response, "LEAVE_ROOM_SUCCESS|\n");
        update_client_room(user_id, -1);
        printf("[INFO] User %d left room\n", user_id);
    } else {
        sprintf(response, "LEAVE_ROOM_FAIL|Not in any room\n");
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_create_auction(int client_socket, char *data) {
    int user_id, room_id, duration;
    char title[200], desc[500];
    double start_price, buy_now_price, min_increment;
    
    sscanf(data, "%d|%d|%[^|]|%[^|]|%lf|%lf|%lf|%d",
           &user_id, &room_id, title, desc, &start_price, 
           &buy_now_price, &min_increment, &duration);
    
    int auction_id = db_create_auction(user_id, room_id, title, desc,
                                       start_price, buy_now_price, 
                                       min_increment, duration);
    
    char response[BUFFER_SIZE];
    if (auction_id > 0) {
        db_update_auction_status(auction_id, "active");
        sprintf(response, "CREATE_AUCTION_SUCCESS|%d|%s\n", auction_id, title);
        printf("[INFO] Auction created: %s (ID: %d)\n", title, auction_id);
    } else {
        sprintf(response, "CREATE_AUCTION_FAIL|Failed to create auction\n");
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_list_auctions(int client_socket, char *data) {
    int room_id;
    sscanf(data, "%d", &room_id);
    
    Auction auctions[MAX_AUCTIONS];
    int count = db_get_auctions_by_room(room_id, auctions, MAX_AUCTIONS);
    
    char response[BUFFER_SIZE * 4] = "AUCTION_LIST|";
    char temp[512];
    time_t now = time(NULL);
    
    for (int i = 0; i < count; i++) {
        int time_left = 0;
        if (strcmp(auctions[i].status, "active") == 0) {
            time_left = auctions[i].end_time - now;
            if (time_left < 0) time_left = 0;
        }
        
        sprintf(temp, "%d;%s;%.2f;%.2f;%d;%d;%s|",
                auctions[i].auction_id,
                auctions[i].title,
                auctions[i].current_price,
                auctions[i].buy_now_price,
                time_left,
                auctions[i].total_bids,
                auctions[i].status);
        strcat(response, temp);
    }
    
    strcat(response, "\n");
    send(client_socket, response, strlen(response), 0);
}

void handle_search_auctions(int client_socket, char *data) {
    SearchFilter filter;
    memset(&filter, 0, sizeof(SearchFilter));
    
    filter.min_price = -1;
    filter.max_price = -1;
    filter.min_time_left = -1;
    filter.max_time_left = -1;
    filter.room_id = -1;
    
    sscanf(data, "%[^|]|%lf|%lf|%d|%d|%[^|]|%d",
           filter.keyword, &filter.min_price, &filter.max_price,
           &filter.min_time_left, &filter.max_time_left,
           filter.status, &filter.room_id);
    
    Auction results[MAX_AUCTIONS];
    int count = db_search_auctions(filter, results, MAX_AUCTIONS);
    
    char response[BUFFER_SIZE * 4] = "SEARCH_RESULTS|";
    char temp[512];
    time_t now = time(NULL);
    
    for (int i = 0; i < count; i++) {
        int time_left = 0;
        if (strcmp(results[i].status, "active") == 0) {
            time_left = results[i].end_time - now;
        }
        
        User seller;
        db_get_user(results[i].seller_id, &seller);
        
        AuctionRoom room;
        db_get_room(results[i].room_id, &room);
        
        sprintf(temp, "%d;%s;%.2f;%.2f;%d;%d;%s;%s;%s|",
                results[i].auction_id, results[i].title,
                results[i].current_price, results[i].buy_now_price,
                time_left, results[i].total_bids, results[i].status,
                seller.username, room.room_name);
        strcat(response, temp);
    }
    
    strcat(response, "\n");
    send(client_socket, response, strlen(response), 0);
    
    printf("[INFO] Search completed: %d results\n", count);
}

void handle_place_bid(int client_socket, char *data) {
    int auction_id, user_id;
    double bid_amount;
    
    sscanf(data, "%d|%d|%lf", &auction_id, &user_id, &bid_amount);
    
    int result = db_place_bid(auction_id, user_id, bid_amount);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        Auction auction;
        db_get_auction(auction_id, &auction);
        
        int time_left = auction.end_time - time(NULL);
        sprintf(response, "BID_SUCCESS|%d|%.2f|%d|%d\n",
                auction_id, bid_amount, auction.total_bids, time_left);
        
        printf("[INFO] Bid placed: User %d, Auction %d, Amount %.2f\n",
               user_id, auction_id, bid_amount);
    } else {
        const char *error_msg;
        switch(result) {
            case -1: error_msg = "Auction not found"; break;
            case -2: error_msg = "Auction not active"; break;
            case -4: error_msg = "Bid too low"; break;
            case -5: error_msg = "Cannot bid on own auction"; break;
            case -6: error_msg = "Insufficient balance"; break;
            default: error_msg = "Unknown error"; break;
        }
        sprintf(response, "BID_FAIL|%s\n", error_msg);
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_buy_now(int client_socket, char *data) {
    int auction_id, user_id;
    sscanf(data, "%d|%d", &auction_id, &user_id);
    
    int result = db_buy_now(auction_id, user_id);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        sprintf(response, "BUY_NOW_SUCCESS|%d\n", auction_id);
        printf("[INFO] Buy now: User %d, Auction %d\n", user_id, auction_id);
    } else {
        const char *error_msg;
        switch(result) {
            case -1: error_msg = "Auction not available"; break;
            case -2: error_msg = "Buy now not available"; break;
            case -3: error_msg = "Insufficient balance"; break;
            default: error_msg = "Unknown error"; break;
        }
        sprintf(response, "BUY_NOW_FAIL|%s\n", error_msg);
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_request(int client_socket, char *buffer) {
    char command[50], data[BUFFER_SIZE];
    
    char *delimiter = strchr(buffer, '|');
    if (delimiter) {
        int cmd_len = delimiter - buffer;
        strncpy(command, buffer, cmd_len);
        command[cmd_len] = '\0';
        strcpy(data, delimiter + 1);
    } else {
        strcpy(command, buffer);
        data[0] = '\0';
    }
    
    printf("[DEBUG] Command: %s\n", command);
    
    if (strcmp(command, "REGISTER") == 0) {
        handle_register(client_socket, data);
    } else if (strcmp(command, "LOGIN") == 0) {
        handle_login(client_socket, data);
    } else if (strcmp(command, "CREATE_ROOM") == 0) {
        handle_create_room(client_socket, data);
    } else if (strcmp(command, "LIST_ROOMS") == 0) {
        handle_list_rooms(client_socket);
    } else if (strcmp(command, "JOIN_ROOM") == 0) {
        handle_join_room(client_socket, data);
    } else if (strcmp(command, "LEAVE_ROOM") == 0) {
        handle_leave_room(client_socket, data);
    } else if (strcmp(command, "CREATE_AUCTION") == 0) {
        handle_create_auction(client_socket, data);
    } else if (strcmp(command, "LIST_AUCTIONS") == 0) {
        handle_list_auctions(client_socket, data);
    } else if (strcmp(command, "SEARCH_AUCTIONS") == 0) {
        handle_search_auctions(client_socket, data);
    } else if (strcmp(command, "PLACE_BID") == 0) {
        handle_place_bid(client_socket, data);
    } else if (strcmp(command, "BUY_NOW") == 0) {
        handle_buy_now(client_socket, data);
    } else {
        char response[256];
        sprintf(response, "ERROR|Unknown command: %s\n", command);
        send(client_socket, response, strlen(response), 0);
    }
}
EOF

# Create network.h
cat > server/network.h << 'EOF'
#ifndef NETWORK_H
#define NETWORK_H

#include "../shared/types.h"
#include "../shared/config.h"
#include <pthread.h>

extern ClientSession g_clients[MAX_CLIENTS];
extern pthread_mutex_t g_client_mutex;

void init_clients();
int add_client_session(int socket, int user_id);
void remove_client_session(int socket);
void update_client_room(int user_id, int room_id);
ClientSession* find_client_by_socket(int socket);
void* handle_client(void *arg);

#endif
EOF

# Create network.c
cat > server/network.c << 'EOF'
#include "network.h"
#include "handlers.h"
#include "database.h"
#include <stdio.h>
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
EOF

# Create main.c
cat > server/main.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "database.h"
#include "network.h"
#include "../shared/config.h"

int main() {
    printf("==============================================\n");
    printf("   ONLINE AUCTION SYSTEM - MODULAR VERSION\n");
    printf("==============================================\n\n");
    
    if (db_init() < 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    
    init_clients();
    
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        db_close();
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        db_close();
        return 1;
    }
    
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        db_close();
        return 1;
    }
    
    printf("[INFO] Server listening on port %d\n", SERVER_PORT);
    printf("[INFO] Database: %s\n", DATABASE_FILE);
    printf("[INFO] Waiting for connections...\n\n");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        pthread_t thread_id;
        int *sock = malloc(sizeof(int));
        *sock = client_socket;
        
        if (pthread_create(&thread_id, NULL, handle_client, sock) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            free(sock);
            continue;
        }
        
        pthread_detach(thread_id);
    }
    
    close(server_socket);
    db_close();
    
    return 0;
}
EOF

echo "✅ All server files created!"
