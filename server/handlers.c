
#include "handlers.h"
#include "database.h"
#include "network.h"
#include "broadcast.h"
#include "../shared/config.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

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
        
        kick_existing_session(user_id);
        
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
        
        // BROADCAST NEW ROOM
        User creator;
        if (db_get_user(creator_id, &creator) == 0) {
            broadcast_room_created(name, creator.username, client_socket);
        }
        
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
        
        // UPDATE CLIENT ROOM FIRST!
        update_client_room(user_id, room_id);
        
        // THEN BROADCAST JOIN
        User user;
        if (db_get_user(user_id, &user) == 0) {
            broadcast_join_room(room_id, user.username, client_socket);
        }
        
        sprintf(response, "JOIN_ROOM_SUCCESS|%d|%s\n", room_id, room.room_name);
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
    
    // Get room before leaving
    int old_room = db_get_user_room(user_id);
    
    int result = db_leave_room(user_id);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        // BROADCAST LEAVE
        User user;
        if (old_room > 0 && db_get_user(user_id, &user) == 0) {
            broadcast_leave_room(old_room, user.username, client_socket);
        }
        
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
    
    int room_creator = db_get_room_creator(room_id);
    if (room_creator != user_id) {
        char response[BUFFER_SIZE];
        sprintf(response, "CREATE_AUCTION_FAIL|Only room creator can create auctions\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    int auction_id = db_create_auction(user_id, room_id, title, desc,
                                       start_price, buy_now_price, 
                                       min_increment, duration);
    
    char response[BUFFER_SIZE];
    if (auction_id > 0) {
        db_update_auction_status(auction_id, "active");
        
        // BROADCAST NEW AUCTION WITH DETAILS
       User creator;
        if (db_get_user(user_id, &creator) == 0) {
            char broadcast_msg[BUFFER_SIZE * 2];
            sprintf(broadcast_msg, "NOTIF_AUCTION_NEW|%d|%s|%s|%.2f|%.2f|%.2f|%d\n",
                    auction_id, title, creator.username, 
                    start_price, buy_now_price, min_increment, duration);
            broadcast_to_room(room_id, broadcast_msg, -1);
            printf("[BROADCAST] New auction: %s in room %d\n", title, room_id);
        }
        
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

void handle_view_auction(int client_socket, char *data) {
    int auction_id;
    sscanf(data, "%d", &auction_id);
    
    Auction auction;
    char seller_name[50] = {0};
    char room_name[100] = {0};
    
    if (db_get_full_auction_details(auction_id, &auction, seller_name, room_name) == 0) {
        time_t now = time(NULL);
        int time_left = (auction.end_time > now) ? (auction.end_time - now) : 0;
        
        char current_bidder[50] = "None";
        if (auction.current_bidder_id > 0) {
            User bidder;
            if (db_get_user(auction.current_bidder_id, &bidder) == 0) {
                strncpy(current_bidder, bidder.username, sizeof(current_bidder)-1);
            }
        }
        
        char response[BUFFER_SIZE * 2];
        sprintf(response, "AUCTION_DETAILS|%d|%s|%s|%s|%.2f|%.2f|%.2f|%s|%d|%d|%s|%s\n",
                auction_id, auction.title, auction.description, seller_name,
                auction.start_price, auction.current_price, auction.buy_now_price,
                current_bidder, auction.total_bids, time_left, auction.status, room_name);
        
        send(client_socket, response, strlen(response), 0);
    } else {
        char response[BUFFER_SIZE];
        sprintf(response, "AUCTION_DETAILS_FAIL|Auction not found\n");
        send(client_socket, response, strlen(response), 0);
    }
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
    
    int seller_id = 0, room_id = 0;
    if (db_get_auction_seller_and_room(auction_id, &seller_id, &room_id) != 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "BID_FAIL|-1|Auction not found\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    if (seller_id == user_id) {
        char response[BUFFER_SIZE];
        sprintf(response, "BID_FAIL|-5|You cannot bid on your own auction\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    Auction auction;
    if (db_get_auction_with_room(auction_id, &auction, &room_id) != 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "BID_FAIL|-1|Auction not found\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    if (strcmp(auction.status, "active") != 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "BID_FAIL|-2|Auction is not active\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    time_t now = time(NULL);
    int time_left = auction.end_time - now;
    if (time_left <= 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "BID_FAIL|-2|Auction has ended\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    int result = db_place_bid(auction_id, user_id, bid_amount);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        db_get_auction_with_room(auction_id, &auction, &room_id);
        
        // ANTI-SNIPE: If bid with < 30s left, extend to 30s
        time_left = auction.end_time - now;
        if (time_left < 30 && time_left > 0) {
            int extend_by = 30 - time_left;
            sqlite3_stmt *stmt;
            const char *sql = "UPDATE auctions SET end_time = end_time + ? WHERE auction_id = ?";
            if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, extend_by);
                sqlite3_bind_int(stmt, 2, auction_id);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                printf("[ANTI-SNIPE] Extended auction %d by %ds\n", auction_id, extend_by);
            }
            time_left = 30;
        }
        
        // BROADCAST TO ALL (including bidder)
        User bidder;
        if (db_get_user(user_id, &bidder) == 0) {
            broadcast_new_bid(room_id, auction_id, auction.title, 
                            bidder.username, bid_amount, time_left);
            printf("[BID] User %s bid %.0f on auction %d\n", bidder.username, bid_amount, auction_id);
        }
        
        sprintf(response, "BID_SUCCESS|%d|%.2f|%d|%d\n", 
                auction_id, bid_amount, auction.total_bids, time_left);
    } else {
        sprintf(response, "BID_FAIL|%d|Bid failed\n", result);
    }
    
    send(client_socket, response, strlen(response), 0);
}
void handle_buy_now(int client_socket, char *data) {
    int auction_id, user_id;
    sscanf(data, "%d|%d", &auction_id, &user_id);
    
    int result = db_buy_now(auction_id, user_id);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        // BROADCAST WINNER
        Auction auction;
        int room_id = 0;
        if (db_get_auction_with_room(auction_id, &auction, &room_id) == 0) {
            User winner;
            if (db_get_user(user_id, &winner) == 0) {
                broadcast_auction_winner(room_id, auction_id, auction.title,
                                       winner.username, auction.buy_now_price, "Buy Now");
            }
        }
        
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

void handle_delete_auction(int client_socket, char *data) {
    int auction_id, user_id;
    sscanf(data, "%d|%d", &auction_id, &user_id);
    
    printf("[DEBUG] Delete auction request: auction_id=%d, user_id=%d\n", auction_id, user_id);
    
    // Get auction details
    Auction auction;
    if (db_get_auction(auction_id, &auction) != 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "DELETE_FAIL|Auction not found\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Check if user is the seller
    if (auction.seller_id != user_id) {
        char response[BUFFER_SIZE];
        sprintf(response, "DELETE_FAIL|Only the creator can delete this auction\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Check status
    if (strcmp(auction.status, "active") != 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "DELETE_FAIL|Can only delete active auctions\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Check for bids
    if (auction.total_bids > 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "DELETE_FAIL|Cannot delete auction with bids\n");
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Delete auction
    int result = db_delete_auction(auction_id, user_id);
    
    char response[BUFFER_SIZE];
    if (result == 0) {
        // Broadcast deletion to room
        broadcast_auction_deleted(auction.room_id, auction_id, auction.title, client_socket);
        
        sprintf(response, "DELETE_SUCCESS|Auction deleted successfully\n");
        printf("[INFO] User %d deleted auction %d\n", user_id, auction_id);
    } else {
        sprintf(response, "DELETE_FAIL|Failed to delete auction\n");
    }
    
    send(client_socket, response, strlen(response), 0);
}

void handle_bid_history(int client_socket, char *data) {
    int auction_id;
    sscanf(data, "%d", &auction_id);
    
    Bid bids[100];
    int count = db_get_bid_history(auction_id, bids, 100);
    
    char response[BUFFER_SIZE * 4];
    strcpy(response, "BID_HISTORY|");
    
    for (int i = 0; i < count; i++) {
        User user;
        char username[50] = "Unknown";
        if (db_get_user(bids[i].user_id, &user) == 0) {
            strncpy(username, user.username, sizeof(username)-1);
        }
        
        char bid_str[200];
        sprintf(bid_str, "%d;%s;%.2f;%ld|", 
                bids[i].bid_id, username, bids[i].bid_amount, bids[i].bid_time);
        strcat(response, bid_str);
    }
    
    strcat(response, "\n");
    send(client_socket, response, strlen(response), 0);
}

void handle_auction_history(int client_socket, char *data) {
    int user_id;
    sscanf(data, "%d", &user_id);
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT a.auction_id, a.title, a.start_price, a.current_price, "
                     "a.status, a.winner_id, a.win_method, u.username as winner "
                     "FROM auctions a "
                     "LEFT JOIN users u ON a.winner_id = u.user_id "
                     "WHERE a.status = 'ended' AND (a.seller_id = ? OR a.winner_id = ?) "
                     "ORDER BY a.end_time DESC LIMIT 20";
    
    char response[BUFFER_SIZE * 4];
    strcpy(response, "AUCTION_HISTORY|");
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, user_id);
        sqlite3_bind_int(stmt, 2, user_id);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int auction_id = sqlite3_column_int(stmt, 0);
            const char *title = (const char*)sqlite3_column_text(stmt, 1);
            double start_price = sqlite3_column_double(stmt, 2);
            double final_price = sqlite3_column_double(stmt, 3);
            const char *winner = (const char*)sqlite3_column_text(stmt, 7);
            const char *method = (const char*)sqlite3_column_text(stmt, 6);
            
            char entry[300];
            sprintf(entry, "%d;%s;%.2f;%.2f;%s;%s|",
                    auction_id, title, start_price, final_price,
                    winner ? winner : "None", method ? method : "none");
            strcat(response, entry);
        }
        
        sqlite3_finalize(stmt);
    }
    
    strcat(response, "\n");
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
    } else if (strcmp(command, "VIEW_AUCTION") == 0) {
        handle_view_auction(client_socket, data);
    } else if (strcmp(command, "SEARCH_AUCTIONS") == 0) {
        handle_search_auctions(client_socket, data);
    } else if (strcmp(command, "PLACE_BID") == 0) {
        handle_place_bid(client_socket, data);
    } else if (strcmp(command, "BUY_NOW") == 0) {
        handle_buy_now(client_socket, data);
    } else if (strcmp(command, "BID_HISTORY") == 0) {
        handle_bid_history(client_socket, data);
    } else if (strcmp(command, "AUCTION_HISTORY") == 0) {
        handle_auction_history(client_socket, data);
    } else {
        char response[256];
        sprintf(response, "ERROR|Unknown command: %s\n", command);
        send(client_socket, response, strlen(response), 0);
    }
}