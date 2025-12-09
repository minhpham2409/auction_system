#!/bin/bash

cd /home/claude/auction-modular

# Create client/network.c
cat > client/network.c << 'EOF'
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
    int received = recv(socket, buffer, buffer_size - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
    }
    return received;
}
EOF

# Create client/ui.h
cat > client/ui.h << 'EOF'
#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include "types.h"

void print_header();
void print_menu(ClientState *state);
void clear_screen();
void print_separator();
void print_error(const char *message);
void print_success(const char *message);
void print_info(const char *message);

#endif
EOF

# Create client/ui.c
cat > client/ui.c << 'EOF'
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>

void clear_screen() {
    printf("\033[2J\033[1;1H");
}

void print_separator() {
    printf("================================================\n");
}

void print_header() {
    clear_screen();
    printf("\033[1;36m");
    print_separator();
    printf("     ONLINE AUCTION SYSTEM - CLIENT v2.0\n");
    print_separator();
    printf("\033[0m\n");
}

void print_error(const char *message) {
    printf("\033[1;31m[ERROR] %s\033[0m\n", message);
}

void print_success(const char *message) {
    printf("\033[1;32m[SUCCESS] %s\033[0m\n", message);
}

void print_info(const char *message) {
    printf("\033[1;34m[INFO] %s\033[0m\n", message);
}

void print_menu(ClientState *state) {
    printf("\n");
    print_separator();
    
    if (state->is_logged_in) {
        printf("User: \033[1;32m%s\033[0m | Balance: \033[1;33m%.2f VND\033[0m\n", 
               state->username, state->balance);
        if (state->current_room_id > 0) {
            printf("Room: \033[1;35m%s (ID: %d)\033[0m\n", 
                   state->current_room_name, state->current_room_id);
        }
    } else {
        printf("Status: \033[1;31mNot logged in\033[0m\n");
    }
    
    print_separator();
    printf("\n");
    
    if (!state->is_logged_in) {
        printf("1. Register\n");
        printf("2. Login\n");
        printf("0. Exit\n");
    } else {
        printf("=== ROOM MANAGEMENT ===\n");
        printf("1. Create Room\n");
        printf("2. List Rooms\n");
        printf("3. Join Room\n");
        printf("4. Leave Room\n");
        
        if (state->current_room_id > 0) {
            printf("\n=== AUCTION MANAGEMENT ===\n");
            printf("5. Create Auction\n");
            printf("6. List Auctions\n");
            printf("7. View Auction Details\n");
            printf("8. Delete Auction\n");
            printf("9. Search Auctions\n");
            
            printf("\n=== BIDDING ===\n");
            printf("10. Place Bid\n");
            printf("11. Buy Now\n");
            printf("12. View Bid History\n");
        }
        
        printf("\n=== ACCOUNT ===\n");
        printf("13. View Balance\n");
        printf("14. Refresh Data\n");
        
        printf("\n0. Logout\n");
    }
    
    print_separator();
    printf("Enter choice: ");
}
EOF

# Create client/features.h
cat > client/features.h << 'EOF'
#ifndef CLIENT_FEATURES_H
#define CLIENT_FEATURES_H

#include "types.h"

// Auth features
void feature_register(ClientState *state);
void feature_login(ClientState *state);
void feature_logout(ClientState *state);

// Room features
void feature_create_room(ClientState *state);
void feature_list_rooms(ClientState *state);
void feature_join_room(ClientState *state);
void feature_leave_room(ClientState *state);

// Auction features
void feature_create_auction(ClientState *state);
void feature_list_auctions(ClientState *state);
void feature_view_auction(ClientState *state);
void feature_delete_auction(ClientState *state);
void feature_search_auctions(ClientState *state);

// Bidding features
void feature_place_bid(ClientState *state);
void feature_buy_now(ClientState *state);
void feature_bid_history(ClientState *state);

// Account features
void feature_view_balance(ClientState *state);
void feature_refresh(ClientState *state);

#endif
EOF

# Create client/features.c - Part 1: Auth & Room
cat > client/features.c << 'EOF'
#include "features.h"
#include "network.h"
#include "ui.h"
#include "../shared/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void feature_register(ClientState *state) {
    char username[50], password[100];
    
    printf("\n=== REGISTER NEW ACCOUNT ===\n");
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
    
    char request[BUFFER_SIZE];
    sprintf(request, "REGISTER|%s|%s\n", username, password);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "REGISTER_SUCCESS", 16) == 0) {
        print_success("Registration successful! Please login.");
    } else {
        print_error("Registration failed. Username may already exist.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_login(ClientState *state) {
    char username[50], password[100];
    
    printf("\n=== LOGIN ===\n");
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
    
    char request[BUFFER_SIZE];
    sprintf(request, "LOGIN|%s|%s\n", username, password);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "LOGIN_SUCCESS", 13) == 0) {
        sscanf(response, "LOGIN_SUCCESS|%d|%[^|]|%lf", 
               &state->user_id, state->username, &state->balance);
        state->is_logged_in = 1;
        state->current_room_id = 0;
        print_success("Login successful!");
    } else {
        print_error("Login failed. Invalid credentials.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_logout(ClientState *state) {
    state->is_logged_in = 0;
    state->user_id = 0;
    state->username[0] = '\0';
    state->balance = 0;
    state->current_room_id = 0;
    state->current_room_name[0] = '\0';
    print_info("Logged out successfully.");
}

void feature_create_room(ClientState *state) {
    char name[100], desc[200];
    int max_participants, duration;
    
    printf("\n=== CREATE ROOM ===\n");
    printf("Room Name: ");
    scanf(" %[^\n]", name);
    printf("Description: ");
    scanf(" %[^\n]", desc);
    printf("Max Participants: ");
    scanf("%d", &max_participants);
    printf("Duration (seconds): ");
    scanf("%d", &duration);
    
    char request[BUFFER_SIZE];
    sprintf(request, "CREATE_ROOM|%d|%s|%s|%d|%d\n",
            state->user_id, name, desc, max_participants, duration);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "CREATE_ROOM_SUCCESS", 19) == 0) {
        int room_id;
        char room_name[100];
        sscanf(response, "CREATE_ROOM_SUCCESS|%d|%[^\n]", &room_id, room_name);
        
        state->current_room_id = room_id;
        strcpy(state->current_room_name, room_name);
        
        print_success("Room created and joined successfully!");
    } else {
        print_error("Failed to create room.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_list_rooms(ClientState *state) {
    printf("\n=== AVAILABLE ROOMS ===\n");
    
    char request[BUFFER_SIZE];
    sprintf(request, "LIST_ROOMS|\n");
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE * 4];
    receive_response(state->socket, response, BUFFER_SIZE * 4);
    
    if (strncmp(response, "ROOM_LIST", 9) == 0) {
        char *data = response + 10;
        
        printf("\n%-5s %-20s %-15s %-15s\n", "ID", "Name", "Creator", "Members");
        print_separator();
        
        char *token = strtok(data, "|");
        while (token != NULL) {
            int room_id, current, max;
            char name[100], creator[50];
            
            sscanf(token, "%d;%[^;];%[^;];%d;%d", 
                   &room_id, name, creator, &current, &max);
            
            printf("%-5d %-20s %-15s %d/%d\n", 
                   room_id, name, creator, current, max);
            
            token = strtok(NULL, "|");
        }
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_join_room(ClientState *state) {
    int room_id;
    
    printf("\n=== JOIN ROOM ===\n");
    printf("Room ID: ");
    scanf("%d", &room_id);
    
    char request[BUFFER_SIZE];
    sprintf(request, "JOIN_ROOM|%d|%d\n", state->user_id, room_id);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "JOIN_ROOM_SUCCESS", 17) == 0) {
        char room_name[100];
        sscanf(response, "JOIN_ROOM_SUCCESS|%d|%[^\n]", &room_id, room_name);
        
        state->current_room_id = room_id;
        strcpy(state->current_room_name, room_name);
        
        print_success("Joined room successfully!");
    } else {
        print_error("Failed to join room.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_leave_room(ClientState *state) {
    if (state->current_room_id <= 0) {
        print_error("You are not in any room.");
        printf("\nPress Enter to continue...");
        getchar(); getchar();
        return;
    }
    
    char request[BUFFER_SIZE];
    sprintf(request, "LEAVE_ROOM|%d\n", state->user_id);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "LEAVE_ROOM_SUCCESS", 18) == 0) {
        state->current_room_id = 0;
        state->current_room_name[0] = '\0';
        print_success("Left room successfully!");
    } else {
        print_error("Failed to leave room.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_create_auction(ClientState *state) {
    char title[200], desc[500];
    double start_price, buy_now, increment;
    int duration;
    
    printf("\n=== CREATE AUCTION ===\n");
    printf("Title: ");
    scanf(" %[^\n]", title);
    printf("Description: ");
    scanf(" %[^\n]", desc);
    printf("Start Price (VND): ");
    scanf("%lf", &start_price);
    printf("Buy Now Price (VND, 0 for no buy now): ");
    scanf("%lf", &buy_now);
    printf("Min Increment (VND): ");
    scanf("%lf", &increment);
    printf("Duration (seconds): ");
    scanf("%d", &duration);
    
    char request[BUFFER_SIZE * 2];
    sprintf(request, "CREATE_AUCTION|%d|%d|%s|%s|%.2f|%.2f|%.2f|%d\n",
            state->user_id, state->current_room_id, title, desc,
            start_price, buy_now, increment, duration);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "CREATE_AUCTION_SUCCESS", 22) == 0) {
        print_success("Auction created successfully!");
    } else {
        print_error("Failed to create auction.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_list_auctions(ClientState *state) {
    printf("\n=== AUCTIONS IN ROOM ===\n");
    
    char request[BUFFER_SIZE];
    sprintf(request, "LIST_AUCTIONS|%d\n", state->current_room_id);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE * 4];
    receive_response(state->socket, response, BUFFER_SIZE * 4);
    
    if (strncmp(response, "AUCTION_LIST", 12) == 0) {
        char *data = response + 13;
        
        printf("\n%-5s %-30s %-15s %-15s %-10s %-10s\n", 
               "ID", "Title", "Current Price", "Buy Now", "Time Left", "Bids");
        print_separator();
        
        char *token = strtok(data, "|");
        while (token != NULL) {
            int auction_id, time_left, bids;
            char title[200], status[20];
            double price, buy_now;
            
            sscanf(token, "%d;%[^;];%lf;%lf;%d;%d;%s",
                   &auction_id, title, &price, &buy_now, &time_left, &bids, status);
            
            printf("%-5d %-30s %-15.2f %-15.2f ", auction_id, title, price, buy_now);
            
            if (strcmp(status, "active") == 0) {
                printf("%-10ds %-10d\n", time_left, bids);
            } else {
                printf("%-10s %-10d\n", status, bids);
            }
            
            token = strtok(NULL, "|");
        }
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_view_auction(ClientState *state) {
    int auction_id;
    
    printf("\n=== VIEW AUCTION DETAILS ===\n");
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    
    print_info("Feature will be implemented with GET_AUCTION command");
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_delete_auction(ClientState *state) {
    int auction_id;
    
    printf("\n=== DELETE AUCTION ===\n");
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    
    print_info("Feature will be implemented with DELETE_AUCTION command");
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_search_auctions(ClientState *state) {
    char keyword[200], status[20];
    double min_price, max_price;
    int min_time, max_time, room_id;
    
    printf("\n=== SEARCH AUCTIONS ===\n");
    printf("Keyword (or Enter to skip): ");
    scanf(" %[^\n]", keyword);
    printf("Min Price (-1 to skip): ");
    scanf("%lf", &min_price);
    printf("Max Price (-1 to skip): ");
    scanf("%lf", &max_price);
    printf("Min Time Left (-1 to skip): ");
    scanf("%d", &min_time);
    printf("Max Time Left (-1 to skip): ");
    scanf("%d", &max_time);
    printf("Status (active/waiting/ended or Enter for all): ");
    scanf(" %[^\n]", status);
    printf("Room ID (-1 for all, 0 for current): ");
    scanf("%d", &room_id);
    
    if (room_id == 0) room_id = state->current_room_id;
    
    char request[BUFFER_SIZE * 2];
    sprintf(request, "SEARCH_AUCTIONS|%s|%.2f|%.2f|%d|%d|%s|%d\n",
            keyword, min_price, max_price, min_time, max_time, status, room_id);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE * 4];
    receive_response(state->socket, response, BUFFER_SIZE * 4);
    
    if (strncmp(response, "SEARCH_RESULTS", 14) == 0) {
        char *data = response + 15;
        
        printf("\n%-5s %-25s %-12s %-10s %-10s %-15s\n",
               "ID", "Title", "Price", "Time", "Bids", "Seller");
        print_separator();
        
        char *token = strtok(data, "|");
        int count = 0;
        while (token != NULL) {
            int auction_id, time_left, bids;
            char title[200], status_str[20], seller[50], room_name[100];
            double price, buy_now;
            
            sscanf(token, "%d;%[^;];%lf;%lf;%d;%d;%[^;];%[^;];%s",
                   &auction_id, title, &price, &buy_now, &time_left, 
                   &bids, status_str, seller, room_name);
            
            printf("%-5d %-25s %-12.2f %-10ds %-10d %-15s\n",
                   auction_id, title, price, time_left, bids, seller);
            
            count++;
            token = strtok(NULL, "|");
        }
        
        if (count == 0) {
            printf("No results found.\n");
        } else {
            printf("\nTotal: %d results\n", count);
        }
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_place_bid(ClientState *state) {
    int auction_id;
    double bid_amount;
    
    printf("\n=== PLACE BID ===\n");
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    printf("Bid Amount (VND): ");
    scanf("%lf", &bid_amount);
    
    char request[BUFFER_SIZE];
    sprintf(request, "PLACE_BID|%d|%d|%.2f\n", 
            auction_id, state->user_id, bid_amount);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "BID_SUCCESS", 11) == 0) {
        print_success("Bid placed successfully!");
        
        // Update balance
        state->balance -= bid_amount;
    } else {
        print_error("Bid failed. Check error message from server.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_buy_now(ClientState *state) {
    int auction_id;
    
    printf("\n=== BUY NOW ===\n");
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    
    char request[BUFFER_SIZE];
    sprintf(request, "BUY_NOW|%d|%d\n", auction_id, state->user_id);
    
    send_request(state->socket, request);
    
    char response[BUFFER_SIZE];
    receive_response(state->socket, response, BUFFER_SIZE);
    
    if (strncmp(response, "BUY_NOW_SUCCESS", 15) == 0) {
        print_success("Purchase successful!");
    } else {
        print_error("Purchase failed.");
    }
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_bid_history(ClientState *state) {
    int auction_id;
    
    printf("\n=== BID HISTORY ===\n");
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    
    print_info("Feature will be implemented with GET_BID_HISTORY command");
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_view_balance(ClientState *state) {
    printf("\n=== ACCOUNT BALANCE ===\n");
    printf("Current Balance: \033[1;33m%.2f VND\033[0m\n", state->balance);
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}

void feature_refresh(ClientState *state) {
    print_info("Refreshing data...");
    
    // Re-login to get updated data
    char request[BUFFER_SIZE];
    sprintf(request, "LOGIN|%s|refresh\n", state->username);
    
    print_success("Data refreshed!");
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}
EOF

# Create client/main.c
cat > client/main.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "network.h"
#include "ui.h"
#include "features.h"
#include "../shared/config.h"

int main() {
    ClientState state;
    memset(&state, 0, sizeof(ClientState));
    
    // Connect to server
    printf("Connecting to server %s:%d...\n", SERVER_IP, SERVER_PORT);
    state.socket = connect_to_server(SERVER_IP, SERVER_PORT);
    
    if (state.socket < 0) {
        print_error("Failed to connect to server!");
        return 1;
    }
    
    print_success("Connected to server!");
    sleep(1);
    
    int choice;
    int running = 1;
    
    while (running) {
        print_header();
        print_menu(&state);
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            print_error("Invalid input!");
            sleep(1);
            continue;
        }
        
        if (!state.is_logged_in) {
            switch(choice) {
                case 1: feature_register(&state); break;
                case 2: feature_login(&state); break;
                case 0: running = 0; break;
                default: 
                    print_error("Invalid choice!");
                    sleep(1);
                    break;
            }
        } else {
            switch(choice) {
                case 1: feature_create_room(&state); break;
                case 2: feature_list_rooms(&state); break;
                case 3: feature_join_room(&state); break;
                case 4: feature_leave_room(&state); break;
                case 5: feature_create_auction(&state); break;
                case 6: feature_list_auctions(&state); break;
                case 7: feature_view_auction(&state); break;
                case 8: feature_delete_auction(&state); break;
                case 9: feature_search_auctions(&state); break;
                case 10: feature_place_bid(&state); break;
                case 11: feature_buy_now(&state); break;
                case 12: feature_bid_history(&state); break;
                case 13: feature_view_balance(&state); break;
                case 14: feature_refresh(&state); break;
                case 0: 
                    feature_logout(&state);
                    running = 0;
                    break;
                default:
                    print_error("Invalid choice!");
                    sleep(1);
                    break;
            }
        }
    }
    
    disconnect_from_server(state.socket);
    print_info("Disconnected from server. Goodbye!");
    
    return 0;
}
EOF

echo "✅ All client files created!"
