#ifndef BROADCAST_H
#define BROADCAST_H

#include "../shared/types.h"

void broadcast_to_room(int room_id, const char *message, int exclude_socket);
void broadcast_to_all_users(const char *message, int exclude_socket);
void broadcast_join_room(int room_id, const char *username, int exclude_socket);
void broadcast_leave_room(int room_id, const char *username, int exclude_socket);
void broadcast_room_created(const char *room_name, const char *creator, int exclude_socket);
void broadcast_auction_created(int room_id, const char *title, const char *creator, int exclude_socket);
void broadcast_auction_deleted(int room_id, int auction_id, const char *title, int exclude_socket);
void broadcast_new_bid(int room_id, int auction_id, const char *title, const char *bidder, double amount, int time_left);
void broadcast_auction_winner(int room_id, int auction_id, const char *title, const char *winner, double price, const char *method);
void broadcast_30s_warning(int room_id, int auction_id, const char *title, int time_left);

#endif