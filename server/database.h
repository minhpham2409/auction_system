#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "../shared/types.h"

// Database connection
extern sqlite3 *g_db;

// Initialize database
int db_init();
void db_close();

// User operations
int db_create_user(const char *username, const char *password, double balance);
int db_authenticate_user(const char *username, const char *password);
int db_get_user(int user_id, User *user);
int db_update_balance(int user_id, double amount);

// Room operations
int db_create_room(int creator_id, const char *name, const char *desc, 
                   int max_participants, int duration);
int db_get_room(int room_id, AuctionRoom *room);
int db_get_all_rooms(AuctionRoom rooms[], int max_count);
int db_join_room(int user_id, int room_id);
int db_leave_room(int user_id);
int db_get_user_room(int user_id);

// Auction operations
int db_create_auction(int seller_id, int room_id, const char *title,
                      const char *desc, double start_price, double buy_now_price,
                      double min_increment, int duration);
int db_get_auction(int auction_id, Auction *auction);
int db_get_auctions_by_room(int room_id, Auction auctions[], int max_count);
int db_delete_auction(int auction_id, int user_id);
int db_search_auctions(SearchFilter filter, Auction results[], int max_results);
int db_update_auction_status(int auction_id, const char *status);

// Bid operations
int db_place_bid(int auction_id, int user_id, double bid_amount);
int db_buy_now(int auction_id, int user_id);
int db_get_bid_history(int auction_id, Bid bids[], int max_count);

// v2.0 Additional functions
int db_get_room_creator(int room_id);
int db_get_auction_with_room(int auction_id, Auction *auction, int *room_id);
int db_get_auction_seller_and_room(int auction_id, int *seller_id, int *room_id);
int db_get_full_auction_details(int auction_id, Auction *auction, char *seller_name, char *room_name);
int db_check_auction_expired(int auction_id);
int db_end_auction(int auction_id, int winner_id, const char *method);
int db_get_all_active_auctions(Auction *auctions, int max_count);

#endif  