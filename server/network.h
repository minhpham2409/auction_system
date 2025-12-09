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
void kick_existing_session(int user_id);

#endif
