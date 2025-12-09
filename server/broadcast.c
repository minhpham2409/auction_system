
#include "broadcast.h"
#include "network.h"
#include "database.h"
#include "../shared/config.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void broadcast_to_room(int room_id, const char *message, int exclude_socket) {
    pthread_mutex_lock(&g_client_mutex);
    
    printf("[BROADCAST] To room %d: %s", room_id, message);
    int sent_count = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].is_active && 
            g_clients[i].current_room_id == room_id &&
            g_clients[i].socket != exclude_socket) {
            
            send(g_clients[i].socket, message, strlen(message), 0);
            sent_count++;
        }
    }
    
    printf("[BROADCAST] Sent to %d clients in room %d\n", sent_count, room_id);
    pthread_mutex_unlock(&g_client_mutex);
}

void broadcast_to_all_users(const char *message, int exclude_socket) {
    pthread_mutex_lock(&g_client_mutex);
    
    int sent_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].is_active && g_clients[i].socket != exclude_socket) {
            send(g_clients[i].socket, message, strlen(message), 0);
            sent_count++;
        }
    }
    
    printf("[BROADCAST] Sent to %d users (all)\n", sent_count);
    pthread_mutex_unlock(&g_client_mutex);
}

void broadcast_join_room(int room_id, const char *username, int exclude_socket) {
    // Get user_id from username
    int user_id = -1;
    User user;
    // Simple lookup - you may want to add db_get_user_by_name()
    for (int i = 1; i < 1000; i++) {
        if (db_get_user(i, &user) == 0 && strcmp(user.username, username) == 0) {
            user_id = i;
            break;
        }
    }
    
    char message[BUFFER_SIZE];
    sprintf(message, "NOTIF_JOIN|%d|%s\n", user_id, username);
    broadcast_to_room(room_id, message, exclude_socket);
}

void broadcast_leave_room(int room_id, const char *username, int exclude_socket) {
    int user_id = -1;
    User user;
    for (int i = 1; i < 1000; i++) {
        if (db_get_user(i, &user) == 0 && strcmp(user.username, username) == 0) {
            user_id = i;
            break;
        }
    }
    
    char message[BUFFER_SIZE];
    sprintf(message, "NOTIF_LEAVE|%d|%s\n", user_id, username);
    broadcast_to_room(room_id, message, exclude_socket);
}

void broadcast_room_created(const char *room_name, const char *creator, int exclude_socket) {
    int user_id = -1;
    User user;
    for (int i = 1; i < 1000; i++) {
        if (db_get_user(i, &user) == 0 && strcmp(user.username, creator) == 0) {
            user_id = i;
            break;
        }
    }
    
    char message[BUFFER_SIZE];
    sprintf(message, "NOTIF_ROOM_NEW|%d|%s|%s\n", user_id, room_name, creator);
    broadcast_to_all_users(message, exclude_socket);
}

void broadcast_auction_created(int room_id, const char *title, const char *creator, int exclude_socket) {
    char message[BUFFER_SIZE * 2];
    sprintf(message, "NOTIF_AUCTION_NEW|%s|%s\n", title, creator);
    broadcast_to_room(room_id, message, exclude_socket);
}

void broadcast_auction_deleted(int room_id, int auction_id, const char *title, int exclude_socket) {
    char message[BUFFER_SIZE];
    sprintf(message, "NOTIF_AUCTION_DEL|%d|%s\n", auction_id, title);
    broadcast_to_room(room_id, message, exclude_socket);
}

void broadcast_new_bid(int room_id, int auction_id, const char *title, const char *bidder, double amount, int time_left) {
    int user_id = -1;
    User user;
    for (int i = 1; i < 1000; i++) {
        if (db_get_user(i, &user) == 0 && strcmp(user.username, bidder) == 0) {
            user_id = i;
            break;
        }
    }
    
    char message[BUFFER_SIZE];
    sprintf(message, "NOTIF_BID|%d|%d|%s|%d|%s|%.2f|%d\n",
            auction_id, user_id, title, user_id, bidder, amount, time_left);
    broadcast_to_room(room_id, message, -1);
}

void broadcast_auction_winner(int room_id, int auction_id, const char *title, const char *winner, double price, const char *method) {
    int user_id = -1;
    User user;
    for (int i = 1; i < 1000; i++) {
        if (db_get_user(i, &user) == 0 && strcmp(user.username, winner) == 0) {
            user_id = i;
            break;
        }
    }
    
    char message[BUFFER_SIZE];
    sprintf(message, "NOTIF_WINNER|%d|%d|%s|%d|%s|%.2f|%s\n",
            auction_id, user_id, title, user_id, winner, price, method);
    broadcast_to_room(room_id, message, -1);
}

void broadcast_30s_warning(int room_id, int auction_id, const char *title, int time_left) {
    char message[BUFFER_SIZE];
    sprintf(message, "NOTIF_WARNING|%d|%s|%d\n", auction_id, title, time_left);
    broadcast_to_room(room_id, message, -1);
}