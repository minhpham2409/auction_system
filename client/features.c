#include "features.h"
#include "network.h"
#include "ui.h"
#include "../shared/config.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    if (!state->is_logged_in) {
        print_error("Please login first!");
        return;
    }
    
    int auction_id;
    
    printf("\n=== VIEW AUCTION DETAILS ===\n");
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    getchar();
    
    char request[BUFFER_SIZE];
    sprintf(request, "VIEW_AUCTION|%d\n", auction_id);
    
    send(state->socket, request, strlen(request), 0);
    
    char response[BUFFER_SIZE * 2];
    int n = receive_response(state->socket, response, sizeof(response));
    
    if (n <= 0) {
        print_error("Connection error!");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }
    
    if (strncmp(response, "AUCTION_DETAILS|", 16) != 0) {
        print_error("Auction not found or invalid response!");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }
    
    // Parse response
    int id, total_bids, time_left;
    char title[200], description[500], seller[50], current_bidder[50], status[20], room_name[100];
    double start_price, current_price, buy_now_price;
    
    sscanf(response + 16, "%d|%[^|]|%[^|]|%[^|]|%lf|%lf|%lf|%[^|]|%d|%d|%[^|]|%s",
           &id, title, description, seller, &start_price, &current_price, &buy_now_price,
           current_bidder, &total_bids, &time_left, status, room_name);
    
    system("clear");
    printf("\n╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                      🎪 AUCTION DETAILS                                ║\n");
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ ID:              %-53d ║\n", id);
    printf("║ Title:           %-53s ║\n", title);
    printf("║ Room:            %-53s ║\n", room_name);
    printf("║ Seller:          %-53s ║\n", seller);
    printf("║ Status:          %-53s ║\n", status);
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Start Price:     %43.2f VND ║\n", start_price);
    printf("║ Current Price:   %43.2f VND ║\n", current_price);
    printf("║ Buy Now Price:   %43.2f VND ║\n", buy_now_price);
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Current Bidder:  %-53s ║\n", current_bidder);
    printf("║ Total Bids:      %-53d ║\n", total_bids);
    
    if (strcmp(status, "active") == 0) {
        int hours = time_left / 3600;
        int minutes = (time_left % 3600) / 60;
        int seconds = time_left % 60;
        printf("║ Time Left:       %02d:%02d:%02d                                          ║\n", hours, minutes, seconds);
    }
    
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Description:                                                           ║\n");
    
    // Word wrap description
    char desc_copy[500];
    strncpy(desc_copy, description, sizeof(desc_copy) - 1);
    char *word = strtok(desc_copy, " ");
    char line[70] = "║ ";
    int line_len = 2;
    
    while (word != NULL) {
        int word_len = strlen(word);
        if (line_len + word_len + 1 > 68) {
            // Pad and print current line
            while (line_len < 68) {
                strcat(line, " ");
                line_len++;
            }
            strcat(line, " ║\n");
            printf("%s", line);
            strcpy(line, "║ ");
            line_len = 2;
        }
        strcat(line, word);
        strcat(line, " ");
        line_len += word_len + 1;
        word = strtok(NULL, " ");
    }
    
    // Print last line
    if (line_len > 2) {
        while (line_len < 68) {
            strcat(line, " ");
            line_len++;
        }
        strcat(line, " ║\n");
        printf("%s", line);
    }
    
    printf("╚════════════════════════════════════════════════════════════════════════╝\n");
    
    printf("\nPress Enter to continue...");
    getchar();
}void feature_delete_auction(ClientState *state) {
    if (!state->is_logged_in) {
        print_error("Please login first!");
        return;
    }
    
    if (state->current_room_id <= 0) {
        print_error("You must be in a room!");
        return;
    }
    
    system("clear");
    printf("\n=== DELETE AUCTION ===\n");
    
    int auction_id;
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    getchar();
    
    printf("\n⚠️  WARNING: This action cannot be undone!\n");
    printf("Only auctions with NO bids can be deleted.\n");
    printf("Continue? (y/n): ");
    
    char confirm;
    scanf("%c", &confirm);
    getchar();
    
    if (confirm != 'y' && confirm != 'Y') {
        print_info("Deletion cancelled.");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }
    
    char request[BUFFER_SIZE];
    sprintf(request, "DELETE_AUCTION|%d|%d\n", auction_id, state->user_id);
    
    send(state->socket, request, strlen(request), 0);
    
    char response[BUFFER_SIZE];
    int n = receive_response(state->socket, response, sizeof(response));
    
    if (n <= 0) {
        print_error("Connection error!");
        printf("\nPress Enter to continue...");
        getchar();
        return;
    }
    
    if (strncmp(response, "DELETE_SUCCESS", 14) == 0) {
        print_success("✅ Auction deleted successfully!");
    } else if (strncmp(response, "DELETE_FAIL", 11) == 0) {
        char *error = strchr(response, '|');
        if (error) {
            print_error(error + 1);
        } else {
            print_error("❌ Failed to delete auction!");
        }
    } else {
        print_error("Invalid response from server!");
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}
void feature_search_auctions(ClientState *state) {
    if (!state->is_logged_in) {
        print_error("Please login first!");
        return;
    }
    
    system("clear");
    printf("\n=== SEARCH AUCTIONS ===\n\n");
    
    char keyword[100] = "";
    double min_price = -1, max_price = -1;
    
    printf("Keyword (Enter to skip): ");
    fgets(keyword, sizeof(keyword), stdin);
    keyword[strcspn(keyword, "\n")] = 0;
    
    printf("Min Price (0 to skip): ");
    scanf("%lf", &min_price);
    
    printf("Max Price (0 to skip): ");
    scanf("%lf", &max_price);
    getchar();
    
    if (min_price == 0) min_price = -1;
    if (max_price == 0) max_price = -1;
    
    char request[BUFFER_SIZE];
    sprintf(request, "SEARCH_AUCTIONS|%s|%.2f|%.2f|-1|-1|active|-1\n",
            keyword, min_price, max_price);
    
    send(state->socket, request, strlen(request), 0);
    
    char response[BUFFER_SIZE * 4];
    int n = receive_response(state->socket, response, sizeof(response));
    
    if (n <= 0) {
        print_error("Connection error!");
        return;
    }
    
    if (strncmp(response, "SEARCH_RESULTS|", 15) != 0) {
        print_error("Invalid response!");
        return;
    }
    
    system("clear");
    printf("\n╔══════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          🔍 SEARCH RESULTS                               ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ ID  │ Title              │ Current    │ Buy Now    │ Time   │ Seller    ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════════╣\n");
    
    char *data = response + 15;
    char *entry = strtok(data, "|");
    int count = 0;
    
    while (entry != NULL && strlen(entry) > 0) {
        int auction_id, time_left, total_bids;
        char title[100], status[20], seller[50], room[50];
        double current_price, buy_now;
        
        sscanf(entry, "%d;%[^;];%lf;%lf;%d;%d;%[^;];%[^;];%s",
               &auction_id, title, &current_price, &buy_now,
               &time_left, &total_bids, status, seller, room);
        
        if (strlen(title) > 18) {
            title[15] = '.';
            title[16] = '.';
            title[17] = '.';
            title[18] = '\0';
        }
        
        printf("║ %-3d │ %-18s │ %9.0f  │ %9.0f  │ %5ds │ %-9s ║\n",
               auction_id, title, current_price, buy_now, time_left, seller);
        
        count++;
        entry = strtok(NULL, "|");
    }
    
    if (count == 0) {
        printf("║                         No auctions found                                ║\n");
    }
    
    printf("╚══════════════════════════════════════════════════════════════════════════╝\n");
    printf("\nTotal: %d auctions\n", count);
    printf("\nPress Enter to continue...");
    getchar();
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
    if (!state->is_logged_in) {
        print_error("Please login first!");
        return;
    }
    
    system("clear");
    printf("\n=== BID HISTORY ===\n");
    
    int auction_id;
    printf("Auction ID: ");
    scanf("%d", &auction_id);
    getchar();
    
    char request[BUFFER_SIZE];
    sprintf(request, "BID_HISTORY|%d\n", auction_id);
    
    send(state->socket, request, strlen(request), 0);
    
    char response[BUFFER_SIZE * 4];
    int n = receive_response(state->socket, response, sizeof(response));
    
    if (n <= 0) {
        print_error("Connection error!");
        return;
    }
    
    if (strncmp(response, "BID_HISTORY|", 12) != 0) {
        print_error("Invalid response!");
        return;
    }
    
    system("clear");
    printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║              📊 BID HISTORY - AUCTION #%-3d                      ║\n", auction_id);
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║ Bid ID │ Username        │ Amount          │ Time               ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    
    char *data = response + 12;
    char *entry = strtok(data, "|");
    int count = 0;
    
    while (entry != NULL && strlen(entry) > 0) {
        int bid_id;
        long bid_time;
        char username[50];
        double amount;
        
        sscanf(entry, "%d;%[^;];%lf;%ld", &bid_id, username, &amount, &bid_time);
        
        time_t t = bid_time;
        struct tm *tm_info = localtime(&t);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%H:%M:%S %d/%m", tm_info);
        
        printf("║ %-6d │ %-15s │ %13.0f   │ %-18s ║\n",
               bid_id, username, amount, time_str);
        
        count++;
        entry = strtok(NULL, "|");
    }
    
    if (count == 0) {
        printf("║                      No bids yet                                 ║\n");
    }
    
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("\nTotal: %d bids\n", count);
    printf("\nPress Enter to continue...");
    getchar();
}
void feature_view_balance(ClientState *state) {
    printf("\n=== ACCOUNT BALANCE ===\n");
    printf("Current Balance: \033[1;33m%.2f VND\033[0m\n", state->balance);
    
    printf("\nPress Enter to continue...");
    getchar(); getchar();
}
void feature_auction_history(ClientState *state) {
    if (!state->is_logged_in) {
        print_error("Please login first!");
        return;
    }
    
    char request[BUFFER_SIZE];
    sprintf(request, "AUCTION_HISTORY|%d\n", state->user_id);
    
    send(state->socket, request, strlen(request), 0);
    
    char response[BUFFER_SIZE * 4];
    int n = receive_response(state->socket, response, sizeof(response));
    if (n <= 0) {
        print_error("Connection error!");
        return;
    }
    response[n] = '\0';
    
    if (strncmp(response, "AUCTION_HISTORY|", 16) != 0) {
        print_error("Invalid response from server!");
        return;
    }
    
    system("clear");
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          📜 AUCTION HISTORY                                ║\n");
    printf("╠════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║ ID  │ Title                 │ Start      │ Final      │ Winner   │ Method  ║\n");
    printf("╠════════════════════════════════════════════════════════════════════════════╣\n");
    
    char *data = response + 16;
    char *entry = strtok(data, "|");
    int count = 0;
    
    while (entry != NULL && strlen(entry) > 0) {
        int auction_id;
        char title[100], winner[50], method[20];
        double start_price, final_price;
        
        sscanf(entry, "%d;%[^;];%lf;%lf;%[^;];%s",
               &auction_id, title, &start_price, &final_price, winner, method);
        
        // Truncate long titles
        if (strlen(title) > 20) {
            title[17] = '.';
            title[18] = '.';
            title[19] = '.';
            title[20] = '\0';
        }
        
        printf("║ %-3d │ %-20s │ %9.0f  │ %9.0f  │ %-8s │ %-7s ║\n",
               auction_id, title, start_price, final_price, winner, method);
        
        count++;
        entry = strtok(NULL, "|");
    }
    
    if (count == 0) {
        printf("║                       No auction history found                             ║\n");
    }
    
    printf("╚════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\nTotal: %d auctions\n", count);
    printf("\nPress Enter to continue...");
    getchar();
    getchar();
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
