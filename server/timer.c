#include "../shared/config.h"
#include "database.h"
#include "broadcast.h"
#include "../shared/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

static int timer_running = 1;

void* auction_timer_thread(void *arg) {
    (void)arg;
    
    
    // Track which auctions got 30s warning already
    int warned_auctions[1000] = {0};
    int warned_count = 0;
    
    while (timer_running) {
        sleep(1); // Check every second
        
        Auction auctions[100];
        int count = db_get_all_active_auctions(auctions, 100);
        
        time_t now = time(NULL);
        
        for (int i = 0; i < count; i++) {
            int time_left = auctions[i].end_time - now;
            
            // AUCTION ENDED
            if (time_left <= 0 && strcmp(auctions[i].status, "active") == 0) {
                printf("[TIMER] Auction %d time expired (time_left=%d)\n", 
                       auctions[i].auction_id, time_left);
                
                if (auctions[i].current_bidder_id > 0) {
                    // Has winner
                    db_end_auction(auctions[i].auction_id, auctions[i].current_bidder_id, "Time Ended");
                    
                    User winner;
                    if (db_get_user(auctions[i].current_bidder_id, &winner) == 0) {
                        int room_id = 0;
                        Auction temp;
                        db_get_auction_with_room(auctions[i].auction_id, &temp, &room_id);
                        
                        // BROADCAST WINNER TO ALL IN ROOM
                        broadcast_auction_winner(room_id, auctions[i].auction_id, 
                                               auctions[i].title, winner.username, 
                                               auctions[i].current_price, "Time Ended");
                        
                        printf("[TIMER] ✅ Auction %d ended - Winner: %s (%.0f VND)\n", 
                               auctions[i].auction_id, winner.username, auctions[i].current_price);
                    }
                } else {
                    // No bids
                    db_end_auction(auctions[i].auction_id, 0, "No Bids");
                    
                    // Broadcast no bids message
                    int room_id = 0;
                    Auction temp;
                    db_get_auction_with_room(auctions[i].auction_id, &temp, &room_id);
                    char no_bid_msg[BUFFER_SIZE];
                    sprintf(no_bid_msg, "NOTIF_WINNER|%d|0|%s|0|No Winner|0.00|No Bids\n",
                            auctions[i].auction_id, auctions[i].title);
                    broadcast_to_room(room_id, no_bid_msg, -1);
                    printf("[TIMER] ❌ Auction %d ended - No bids\n", auctions[i].auction_id);
                }
                
                // Reset warning flag for this auction
                for (int j = 0; j < warned_count; j++) {
                    if (warned_auctions[j] == auctions[i].auction_id) {
                        warned_auctions[j] = 0;
                    }
                }
            }
            // 30 SECOND WARNING
            else if (time_left <= 30 && time_left > 0) {
                // Check if already warned
                int already_warned = 0;
                for (int j = 0; j < warned_count; j++) {
                    if (warned_auctions[j] == auctions[i].auction_id) {
                        already_warned = 1;
                        break;
                    }
                }
                
                if (!already_warned) {
                    int room_id = 0;
                    Auction temp;
                    db_get_auction_with_room(auctions[i].auction_id, &temp, &room_id);
                    
                    // BROADCAST WARNING TO ALL IN ROOM
                    broadcast_30s_warning(room_id, auctions[i].auction_id, auctions[i].title, time_left);
                    
                    // Mark as warned
                    if (warned_count < 1000) {
                        warned_auctions[warned_count++] = auctions[i].auction_id;
                    }
                    
                    printf("[TIMER] ⚠️  30s warning for auction %d (%s) - %ds left\n", 
                           auctions[i].auction_id, auctions[i].title, time_left);
                }
            }
        }
    }
    
    printf("[TIMER] Auction timer thread stopped\n");
    return NULL;
}

void start_auction_timer() {
    pthread_t timer_thread;
    if (pthread_create(&timer_thread, NULL, auction_timer_thread, NULL) != 0) {
        fprintf(stderr, "[ERROR] Failed to create timer thread\n");
        return;
    }
    pthread_detach(timer_thread);
    printf("[INFO] Auction timer initialized\n");
}