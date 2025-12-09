#ifndef CLIENT_TYPES_H
#define CLIENT_TYPES_H

// Client state
typedef struct {
    int socket;
    int user_id;
    char username[50];
    double balance;
    int current_room_id;
    char current_room_name[100];
    int is_logged_in;
} ClientState;

#endif
