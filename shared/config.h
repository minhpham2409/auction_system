#ifndef CONFIG_H
#define CONFIG_H

// Network
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 4096

// Limits
#define MAX_USERS 1000
#define MAX_ROOMS 100
#define MAX_AUCTIONS 1000
#define MAX_BIDS 10000
#define MAX_CLIENTS 100

// Database
#define DATABASE_FILE "auction.db"
#define ACTIVITY_LOG_FILE "activity_log.txt"

// Validation
#define MIN_BID_INCREMENT 10000.0
#define MIN_USERNAME_LENGTH 3
#define MIN_PASSWORD_LENGTH 6

#endif
