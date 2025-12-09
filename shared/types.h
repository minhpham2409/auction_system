#ifndef TYPES_H
#define TYPES_H

#include <time.h>

// User structure
typedef struct {
    int user_id;
    char username[50];
    char password[100];
    double balance;
    int is_active;
} User;

// Room structure
typedef struct {
    int room_id;
    char room_name[100];
    char description[200];
    int created_by;
    int max_participants;
    int current_participants;
    int total_auctions;
    time_t created_at;
    time_t end_time;
    char status[20];
} AuctionRoom;

// Auction structure
typedef struct {
    int auction_id;
    int seller_id;
    int room_id;
    char title[200];
    char description[500];
    double start_price;
    double current_price;
    double buy_now_price;
    double min_increment;
    int current_bidder_id;
    time_t start_time;
    time_t end_time;
    int duration;
    int total_bids;
    char status[20];
    int winner_id;
    char win_method[20];
} Auction;

// Bid structure
typedef struct {
    int bid_id;
    int auction_id;
    int user_id;
    double bid_amount;
    time_t bid_time;
} Bid;

// Client session
typedef struct {
    int socket;
    int user_id;
    int current_room_id;
    int is_active;
} ClientSession;

// Search filter
typedef struct {
    char keyword[200];
    double min_price;
    double max_price;
    int min_time_left;
    int max_time_left;
    char status[20];
    int room_id;
} SearchFilter;

#endif
