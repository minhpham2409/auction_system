#ifndef HANDLERS_H
#define HANDLERS_H

void handle_register(int client_socket, char *data);
void handle_login(int client_socket, char *data);
void handle_create_room(int client_socket, char *data);
void handle_list_rooms(int client_socket);
void handle_join_room(int client_socket, char *data);
void handle_leave_room(int client_socket, char *data);
void handle_create_auction(int client_socket, char *data);
void handle_list_auctions(int client_socket, char *data);
void handle_search_auctions(int client_socket, char *data);
void handle_place_bid(int client_socket, char *data);
void handle_buy_now(int client_socket, char *data);
void handle_request(int client_socket, char *buffer);

#endif
