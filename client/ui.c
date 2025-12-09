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
        printf("  15. 📜 Auction History\n");
        printf("\n0. Logout\n");
    }
    
    print_separator();
    printf("Enter choice: ");
}
