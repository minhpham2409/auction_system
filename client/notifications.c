
#include "notifications.h"
#include "../shared/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>

static int notif_running = 1;
static pthread_t notif_thread_id;

void* notification_thread(void *arg) {
    int socket = *(int*)arg;
    char buffer[BUFFER_SIZE * 2];
    
    printf("[NOTIF] Notification listener started\n");
    
    while (notif_running) {
        memset(buffer, 0, sizeof(buffer));
        
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        
        int activity = select(socket + 1, &readfds, NULL, NULL, &tv);
        
        if (activity > 0) {
            int n = recv(socket, buffer, sizeof(buffer) - 1, MSG_PEEK | MSG_DONTWAIT);
            
            if (n > 0) {
                buffer[n] = '\0';
                
                if (strncmp(buffer, "NOTIF_", 6) == 0 || strncmp(buffer, "KICKED|", 7) == 0) {
                    recv(socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
                    buffer[n] = '\0';
                    
                    char *newline = strchr(buffer, '\n');
                    if (newline) *newline = '\0';
                    
                    if (strncmp(buffer, "KICKED|", 7) == 0) {
                        printf("\n\n\033[1;31m╔══════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;31m║  ⚠️  SESSION TERMINATED              ║\033[0m\n");
                        printf("\033[1;31m╚══════════════════════════════════════╝\033[0m\n\n");
                        fflush(stdout);
                        exit(0);
                    }
                    else if (strncmp(buffer, "NOTIF_JOIN|", 11) == 0) {
                        int user_id;
                        char username[50];
                        sscanf(buffer + 11, "%d|%s", &user_id, username);
                        printf("\n\033[1;36m╔══════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;36m║ 🚪 USER JOINED (ID:%d)               ║\033[0m\n", user_id);
                        printf("\033[1;36m║ %s%-36s ║\033[0m\n", "", username);
                        printf("\033[1;36m╚══════════════════════════════════════╝\033[0m\n> ");
                        fflush(stdout);
                    }
                    else if (strncmp(buffer, "NOTIF_LEAVE|", 12) == 0) {
                        int user_id;
                        char username[50];
                        sscanf(buffer + 12, "%d|%s", &user_id, username);
                        printf("\n\033[1;35m╔══════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;35m║ 🚶 USER LEFT (ID:%d)                 ║\033[0m\n", user_id);
                        printf("\033[1;35m║ %s%-36s ║\033[0m\n", "", username);
                        printf("\033[1;35m╚══════════════════════════════════════╝\033[0m\n> ");
                        fflush(stdout);
                    }
                    else if (strncmp(buffer, "NOTIF_ROOM_NEW|", 15) == 0) {
                        int user_id;
                        char room_name[100], creator[50];
                        sscanf(buffer + 15, "%d|%[^|]|%s", &user_id, room_name, creator);
                        printf("\n\033[1;34m╔══════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;34m║ 🏠 NEW ROOM (Creator ID:%d)          ║\033[0m\n", user_id);
                        printf("\033[1;34m║ %s%-36s ║\033[0m\n", "", room_name);
                        printf("\033[1;34m║ By: %-32s ║\033[0m\n", creator);
                        printf("\033[1;34m╚══════════════════════════════════════╝\033[0m\n> ");
                        fflush(stdout);
                    }
                    else if (strncmp(buffer, "NOTIF_AUCTION_NEW|", 18) == 0) {
                        int auction_id, duration;
                        char title[200], creator[50];
                        double start_price, buy_now, min_inc;
                        
                        int parsed = sscanf(buffer + 18, "%d|%[^|]|%[^|]|%lf|%lf|%lf|%d",
                               &auction_id, title, creator, &start_price, &buy_now, &min_inc, &duration);
                        
                        printf("\n\033[1;35m╔════════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;35m║ 🎪 NEW AUCTION (ID:#%d)                ║\033[0m\n", auction_id);
                        printf("\033[1;35m╠════════════════════════════════════════╣\033[0m\n");
                        
                        if (parsed == 7) {
                            printf("\033[1;35m║ Title: %-31s ║\033[0m\n", title);
                            printf("\033[1;35m║ Creator: %-29s ║\033[0m\n", creator);
                            printf("\033[1;35m║ Start:      %19.0f VND ║\033[0m\n", start_price);
                            printf("\033[1;35m║ Buy Now:    %19.0f VND ║\033[0m\n", buy_now);
                            printf("\033[1;35m║ Duration:              %10ds ║\033[0m\n", duration);
                        }
                        
                        printf("\033[1;35m╚════════════════════════════════════════╝\033[0m\n> ");
                        fflush(stdout);
                    }
                    else if (strncmp(buffer, "NOTIF_BID|", 10) == 0) {
                        int auction_id, user_id, time_left;
                        char title[200], bidder[50];
                        double amount;
                        
                        sscanf(buffer + 10, "%d|%d|%[^|]|%*d|%[^|]|%lf|%d",
                               &auction_id, &user_id, title, bidder, &amount, &time_left);
                        
                        printf("\n\033[1;33m╔════════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;33m║ 💰 BID (Auction:#%d User:#%d)          ║\033[0m\n", auction_id, user_id);
                        printf("\033[1;33m╠════════════════════════════════════════╣\033[0m\n");
                        printf("\033[1;33m║ Auction: %-29s ║\033[0m\n", title);
                        printf("\033[1;33m║ Bidder: %-30s ║\033[0m\n", bidder);
                        printf("\033[1;33m║ Amount:     %19.0f VND ║\033[0m\n", amount);
                        printf("\033[1;33m║ Time Left:             %10ds ║\033[0m\n", time_left);
                        printf("\033[1;33m╚════════════════════════════════════════╝\033[0m\n> ");
                        fflush(stdout);
                    }
                    else if (strncmp(buffer, "NOTIF_WINNER|", 13) == 0) {
                        int auction_id, user_id;
                        char title[200], winner[50], method[50];
                        double price;
                        
                        sscanf(buffer + 13, "%d|%d|%[^|]|%*d|%[^|]|%lf|%[^\n]",
                               &auction_id, &user_id, title, winner, &price, method);
                        
                        printf("\n\033[1;32m╔════════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;32m║ 🏆 WINNER (Auction:#%d User:#%d)       ║\033[0m\n", auction_id, user_id);
                        printf("\033[1;32m╠════════════════════════════════════════╣\033[0m\n");
                        printf("\033[1;32m║ Auction: %-29s ║\033[0m\n", title);
                        printf("\033[1;32m║ Winner: %-30s ║\033[0m\n", winner);
                        printf("\033[1;32m║ Price:      %19.0f VND ║\033[0m\n", price);
                        printf("\033[1;32m║ Method: %-30s ║\033[0m\n", method);
                        printf("\033[1;32m╚════════════════════════════════════════╝\033[0m\n> ");
                        fflush(stdout);
                    }
                    else if (strncmp(buffer, "NOTIF_WARNING|", 14) == 0) {
                        int auction_id, time_left;
                        char title[200];
                        
                        sscanf(buffer + 14, "%d|%[^|]|%d", &auction_id, title, &time_left);
                        
                        printf("\n\033[1;31m╔════════════════════════════════════════╗\033[0m\n");
                        printf("\033[1;31m║ ⚠️  WARNING (Auction:#%d)              ║\033[0m\n", auction_id);
                        printf("\033[1;31m╠════════════════════════════════════════╣\033[0m\n");
                        printf("\033[1;31m║ %s%-38s ║\033[0m\n", "", title);
                        printf("\033[1;31m║ Time Left:             %10ds ║\033[0m\n", time_left);
                        printf("\033[1;31m║      🔥 LAST CHANCE TO BID! 🔥         ║\033[0m\n");
                        printf("\033[1;31m╚════════════════════════════════════════╝\033[0m\n> ");
                        fflush(stdout);
                    }
                }
            } else if (n == 0) {
                printf("\n[ERROR] Server disconnected!\n");
                exit(1);
            }
        }
    }
    
    return NULL;
}

void start_notification_listener(int socket) {
    notif_running = 1;
    int *sock_ptr = malloc(sizeof(int));
    *sock_ptr = socket;
    
    if (pthread_create(&notif_thread_id, NULL, notification_thread, sock_ptr) != 0) {
        fprintf(stderr, "[ERROR] Failed to create notification thread\n");
        free(sock_ptr);
    } else {
        pthread_detach(notif_thread_id);
        printf("\033[1;32m[✓] Notifications with IDs enabled!\033[0m\n");
    }
}

void stop_notification_listener() {
    notif_running = 0;
}