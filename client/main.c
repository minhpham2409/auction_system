
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "types.h"
#include "network.h"
#include "ui.h"
#include "features.h"
#include "notifications.h"
#include "../shared/config.h"

int main() {
    ClientState state;
    memset(&state, 0, sizeof(ClientState));
    
    printf("Connecting to server %s:%d...\n", SERVER_IP, SERVER_PORT);
    state.socket = connect_to_server(SERVER_IP, SERVER_PORT);
    
    if (state.socket < 0) {
        print_error("Failed to connect to server!");
        return 1;
    }
    
    print_success("Connected to server!");
    sleep(1);
    
    int choice;
    int running = 1;
    
    while (running) {
        print_header();
        print_menu(&state);
        
        printf("> ");
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            print_error("Invalid input!");
            sleep(1);
            continue;
        }
        
        if (!state.is_logged_in) {
            switch(choice) {
                case 1: 
                    feature_register(&state); 
                    break;
                    
                case 2: 
                    feature_login(&state);
                    // ===== START NOTIFICATION THREAD =====
                    if (state.is_logged_in) {
                        start_notification_listener(state.socket);
                        sleep(1); // Give thread time to start
                    }
                    break;
                    
                case 0: 
                    running = 0; 
                    break;
                    
                default: 
                    print_error("Invalid choice!");
                    sleep(1);
                    break;
            }
        } else {
            switch(choice) {
                case 1: feature_create_room(&state); break;
                case 2: feature_list_rooms(&state); break;
                case 3: feature_join_room(&state); break;
                case 4: feature_leave_room(&state); break;
                case 5: feature_create_auction(&state); break;
                case 6: feature_list_auctions(&state); break;
                case 7: feature_view_auction(&state); break;
                case 8: feature_delete_auction(&state); break;
                case 9: feature_search_auctions(&state); break;
                case 10: feature_place_bid(&state); break;
                case 11: feature_buy_now(&state); break;
                case 12: feature_bid_history(&state); break;
                case 13: feature_view_balance(&state); break;
                case 14: feature_refresh(&state); break;
                case 15: feature_auction_history(&state); break;
                case 0: 
                    feature_logout(&state);
                    stop_notification_listener();
                    running = 0;
                    break;
                default:
                    print_error("Invalid choice!");
                    sleep(1);
                    break;
            }
        }
    }
    
    stop_notification_listener();
    close(state.socket);
    print_info("Disconnected from server. Goodbye!");
    
    return 0;
}