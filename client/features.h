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
void feature_auction_history(ClientState *state);

#endif
