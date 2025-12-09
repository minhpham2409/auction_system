#include "database.h"
#include "../shared/config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

sqlite3 *g_db = NULL;

int db_init() {
    int rc = sqlite3_open(DATABASE_FILE, &g_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }
    
    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    printf("[INFO] Database opened: %s\n", DATABASE_FILE);
    return 0;
}

void db_close() {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

int db_create_user(const char *username, const char *password, double balance) {
    char *sql = "INSERT INTO users (username, password, balance) VALUES (?, ?, ?)";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, balance);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) return -1;
    return sqlite3_last_insert_rowid(g_db);
}

int db_authenticate_user(const char *username, const char *password) {
    char *sql = "SELECT user_id FROM users WHERE username = ? AND password = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    
    int user_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return user_id;
}

int db_get_user(int user_id, User *user) {
    char *sql = "SELECT * FROM users WHERE user_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user->user_id = sqlite3_column_int(stmt, 0);
        strcpy(user->username, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(user->password, (const char*)sqlite3_column_text(stmt, 2));
        user->balance = sqlite3_column_double(stmt, 3);
        user->is_active = sqlite3_column_int(stmt, 4);
        
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_update_balance(int user_id, double amount) {
    char *sql = "UPDATE users SET balance = balance + ? WHERE user_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, amount);
    sqlite3_bind_int(stmt, 2, user_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_create_room(int creator_id, const char *name, const char *desc,
                   int max_participants, int duration) {
    char *sql = "INSERT INTO rooms (room_name, description, created_by, max_participants, end_time) "
                "VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    time_t end_time = time(NULL) + duration;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, desc, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, creator_id);
    sqlite3_bind_int(stmt, 4, max_participants);
    sqlite3_bind_int64(stmt, 5, end_time);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) return -1;
    return sqlite3_last_insert_rowid(g_db);
}

int db_get_room(int room_id, AuctionRoom *room) {
    char *sql = "SELECT * FROM rooms WHERE room_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        room->room_id = sqlite3_column_int(stmt, 0);
        strcpy(room->room_name, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(room->description, (const char*)sqlite3_column_text(stmt, 2));
        room->created_by = sqlite3_column_int(stmt, 3);
        room->max_participants = sqlite3_column_int(stmt, 4);
        room->current_participants = sqlite3_column_int(stmt, 5);
        room->total_auctions = sqlite3_column_int(stmt, 6);
        room->created_at = sqlite3_column_int64(stmt, 7);
        room->end_time = sqlite3_column_int64(stmt, 8);
        strcpy(room->status, (const char*)sqlite3_column_text(stmt, 9));
        
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_get_all_rooms(AuctionRoom rooms[], int max_count) {
    char *sql = "SELECT * FROM rooms WHERE status = 'active' ORDER BY created_at DESC LIMIT ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, max_count);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        rooms[count].room_id = sqlite3_column_int(stmt, 0);
        strcpy(rooms[count].room_name, (const char*)sqlite3_column_text(stmt, 1));
        strcpy(rooms[count].description, (const char*)sqlite3_column_text(stmt, 2));
        rooms[count].created_by = sqlite3_column_int(stmt, 3);
        rooms[count].max_participants = sqlite3_column_int(stmt, 4);
        rooms[count].current_participants = sqlite3_column_int(stmt, 5);
        rooms[count].total_auctions = sqlite3_column_int(stmt, 6);
        rooms[count].created_at = sqlite3_column_int64(stmt, 7);
        rooms[count].end_time = sqlite3_column_int64(stmt, 8);
        strcpy(rooms[count].status, (const char*)sqlite3_column_text(stmt, 9));
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int db_join_room(int user_id, int room_id) {
    sqlite3_exec(g_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    
    char *sql = "SELECT current_participants, max_participants FROM rooms WHERE room_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    
    int current = 0, max = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        current = sqlite3_column_int(stmt, 0);
        max = sqlite3_column_int(stmt, 1);
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    sqlite3_finalize(stmt);
    
    if (current >= max) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -3;
    }
    
    sql = "INSERT OR IGNORE INTO user_rooms (user_id, room_id) VALUES (?, ?)";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, room_id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -4;
    }
    
    sql = "UPDATE rooms SET current_participants = current_participants + 1 WHERE room_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sqlite3_exec(g_db, "COMMIT;", NULL, NULL, NULL);
    return 0;
}

int db_leave_room(int user_id) {
    char *sql = "SELECT room_id FROM user_rooms WHERE user_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    
    int room_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        room_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (room_id == -1) return -1;
    
    sqlite3_exec(g_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    
    sql = "DELETE FROM user_rooms WHERE user_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sql = "UPDATE rooms SET current_participants = current_participants - 1 WHERE room_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sqlite3_exec(g_db, "COMMIT;", NULL, NULL, NULL);
    return 0;
}

int db_get_user_room(int user_id) {
    char *sql = "SELECT room_id FROM user_rooms WHERE user_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    
    int room_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        room_id = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return room_id;
}

int db_create_auction(int seller_id, int room_id, const char *title,
                      const char *desc, double start_price, double buy_now_price,
                      double min_increment, int duration) {
    char *sql = "INSERT INTO auctions (seller_id, room_id, title, description, "
                "start_price, current_price, buy_now_price, min_increment, duration, start_time, end_time) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;
    
    time_t now = time(NULL);
    time_t end_time = now + duration;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, seller_id);
    sqlite3_bind_int(stmt, 2, room_id);
    sqlite3_bind_text(stmt, 3, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, desc, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 5, start_price);
    sqlite3_bind_double(stmt, 6, start_price);
    sqlite3_bind_double(stmt, 7, buy_now_price);
    sqlite3_bind_double(stmt, 8, min_increment);
    sqlite3_bind_int(stmt, 9, duration);
    sqlite3_bind_int64(stmt, 10, now);
    sqlite3_bind_int64(stmt, 11, end_time);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) return -1;
    
    int auction_id = sqlite3_last_insert_rowid(g_db);
    
    sql = "UPDATE rooms SET total_auctions = total_auctions + 1 WHERE room_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return auction_id;
}

int db_get_auction(int auction_id, Auction *auction) {
    char *sql = "SELECT * FROM auctions WHERE auction_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, auction_id);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auction->auction_id = sqlite3_column_int(stmt, 0);
        auction->seller_id = sqlite3_column_int(stmt, 1);
        auction->room_id = sqlite3_column_int(stmt, 2);
        strcpy(auction->title, (const char*)sqlite3_column_text(stmt, 3));
        strcpy(auction->description, (const char*)sqlite3_column_text(stmt, 4));
        auction->start_price = sqlite3_column_double(stmt, 5);
        auction->current_price = sqlite3_column_double(stmt, 6);
        auction->buy_now_price = sqlite3_column_double(stmt, 7);
        auction->min_increment = sqlite3_column_double(stmt, 8);
        auction->current_bidder_id = sqlite3_column_int(stmt, 9);
        auction->start_time = sqlite3_column_int64(stmt, 10);
        auction->end_time = sqlite3_column_int64(stmt, 11);
        auction->duration = sqlite3_column_int(stmt, 12);
        auction->total_bids = sqlite3_column_int(stmt, 13);
        strcpy(auction->status, (const char*)sqlite3_column_text(stmt, 14));
        auction->winner_id = sqlite3_column_int(stmt, 15);
        
        const char *win_method = (const char*)sqlite3_column_text(stmt, 16);
        if (win_method) {
            strcpy(auction->win_method, win_method);
        } else {
            auction->win_method[0] = '\0';
        }
        
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_get_auctions_by_room(int room_id, Auction auctions[], int max_count) {
    char *sql = "SELECT * FROM auctions WHERE room_id = ? AND status != 'deleted' "
                "ORDER BY start_time DESC LIMIT ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_bind_int(stmt, 2, max_count);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        auctions[count].auction_id = sqlite3_column_int(stmt, 0);
        auctions[count].seller_id = sqlite3_column_int(stmt, 1);
        auctions[count].room_id = sqlite3_column_int(stmt, 2);
        strcpy(auctions[count].title, (const char*)sqlite3_column_text(stmt, 3));
        strcpy(auctions[count].description, (const char*)sqlite3_column_text(stmt, 4));
        auctions[count].start_price = sqlite3_column_double(stmt, 5);
        auctions[count].current_price = sqlite3_column_double(stmt, 6);
        auctions[count].buy_now_price = sqlite3_column_double(stmt, 7);
        auctions[count].min_increment = sqlite3_column_double(stmt, 8);
        auctions[count].current_bidder_id = sqlite3_column_int(stmt, 9);
        auctions[count].start_time = sqlite3_column_int64(stmt, 10);
        auctions[count].end_time = sqlite3_column_int64(stmt, 11);
        auctions[count].duration = sqlite3_column_int(stmt, 12);
        auctions[count].total_bids = sqlite3_column_int(stmt, 13);
        strcpy(auctions[count].status, (const char*)sqlite3_column_text(stmt, 14));
        auctions[count].winner_id = sqlite3_column_int(stmt, 15);
        
        const char *win_method = (const char*)sqlite3_column_text(stmt, 16);
        if (win_method) {
            strcpy(auctions[count].win_method, win_method);
        } else {
            auctions[count].win_method[0] = '\0';
        }
        
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int db_delete_auction(int auction_id, int user_id) {
    sqlite3_exec(g_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    
    char *sql = "SELECT seller_id, room_id, status FROM auctions WHERE auction_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, auction_id);
    
    int seller_id = -1, room_id = -1;
    char status[20];
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        seller_id = sqlite3_column_int(stmt, 0);
        room_id = sqlite3_column_int(stmt, 1);
        strcpy(status, (const char*)sqlite3_column_text(stmt, 2));
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    sqlite3_finalize(stmt);
    
    if (strcmp(status, "waiting") != 0) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -2;
    }
    
    sql = "SELECT created_by FROM rooms WHERE room_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    
    int room_creator = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        room_creator = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (seller_id != user_id && room_creator != user_id) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -4;
    }
    
    sql = "UPDATE auctions SET status = 'deleted' WHERE auction_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, auction_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sql = "UPDATE rooms SET total_auctions = total_auctions - 1 WHERE room_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sqlite3_exec(g_db, "COMMIT;", NULL, NULL, NULL);
    return 0;
}

int db_search_auctions(SearchFilter filter, Auction results[], int max_results) {
    char sql[2048] = "SELECT * FROM auctions WHERE status != 'deleted'";
    
    if (filter.room_id >= 0) {
        char temp[64];
        sprintf(temp, " AND room_id = %d", filter.room_id);
        strcat(sql, temp);
    }
    
    if (strlen(filter.status) > 0) {
        char temp[64];
        sprintf(temp, " AND status = '%s'", filter.status);
        strcat(sql, temp);
    }
    
    if (strlen(filter.keyword) > 0) {
        char temp[512];  // Increased buffer size
        sprintf(temp, " AND (title LIKE '%%%s%%' OR description LIKE '%%%s%%')", 
                filter.keyword, filter.keyword);
        strcat(sql, temp);
    }
    
    if (filter.min_price >= 0) {
        char temp[64];
        sprintf(temp, " AND current_price >= %.2f", filter.min_price);
        strcat(sql, temp);
    }
    
    if (filter.max_price >= 0) {
        char temp[64];
        sprintf(temp, " AND current_price <= %.2f", filter.max_price);
        strcat(sql, temp);
    }
    
    strcat(sql, " ORDER BY start_time DESC");
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    
    int count = 0;
    time_t now = time(NULL);
    
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_results) {
        Auction *a = &results[count];
        
        a->auction_id = sqlite3_column_int(stmt, 0);
        a->seller_id = sqlite3_column_int(stmt, 1);
        a->room_id = sqlite3_column_int(stmt, 2);
        strcpy(a->title, (const char*)sqlite3_column_text(stmt, 3));
        strcpy(a->description, (const char*)sqlite3_column_text(stmt, 4));
        a->start_price = sqlite3_column_double(stmt, 5);
        a->current_price = sqlite3_column_double(stmt, 6);
        a->buy_now_price = sqlite3_column_double(stmt, 7);
        a->min_increment = sqlite3_column_double(stmt, 8);
        a->current_bidder_id = sqlite3_column_int(stmt, 9);
        a->start_time = sqlite3_column_int64(stmt, 10);
        a->end_time = sqlite3_column_int64(stmt, 11);
        a->duration = sqlite3_column_int(stmt, 12);
        a->total_bids = sqlite3_column_int(stmt, 13);
        strcpy(a->status, (const char*)sqlite3_column_text(stmt, 14));
        a->winner_id = sqlite3_column_int(stmt, 15);
        
        const char *win_method = (const char*)sqlite3_column_text(stmt, 16);
        if (win_method) {
            strcpy(a->win_method, win_method);
        } else {
            a->win_method[0] = '\0';
        }
        
        if (strcmp(a->status, "active") == 0) {
            int time_left = a->end_time - now;
            
            if (filter.min_time_left >= 0 && time_left < filter.min_time_left) {
                continue;
            }
            if (filter.max_time_left >= 0 && time_left > filter.max_time_left) {
                continue;
            }
        }
        
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int db_update_auction_status(int auction_id, const char *status) {
    char *sql = "UPDATE auctions SET status = ? WHERE auction_id = ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, status, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, auction_id);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_place_bid(int auction_id, int user_id, double bid_amount) {
    sqlite3_exec(g_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    
    char *sql = "SELECT current_price, seller_id, status FROM auctions WHERE auction_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, auction_id);
    
    double current_price = 0;
    int seller_id = -1;
    char status[20];
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        current_price = sqlite3_column_double(stmt, 0);
        seller_id = sqlite3_column_int(stmt, 1);
        strcpy(status, (const char*)sqlite3_column_text(stmt, 2));
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    sqlite3_finalize(stmt);
    
    if (strcmp(status, "active") != 0) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -2;
    }
    
    if (seller_id == user_id) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -5;
    }
    
    if (bid_amount <= current_price) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -4;
    }
    
    sql = "SELECT balance FROM users WHERE user_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    
    double balance = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        balance = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (balance < bid_amount) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -6;
    }
    
    sql = "UPDATE auctions SET current_price = ?, current_bidder_id = ?, total_bids = total_bids + 1 WHERE auction_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, bid_amount);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_int(stmt, 3, auction_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sql = "INSERT INTO bids (auction_id, user_id, bid_amount) VALUES (?, ?, ?)";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, auction_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_bind_double(stmt, 3, bid_amount);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sqlite3_exec(g_db, "COMMIT;", NULL, NULL, NULL);
    return 0;
}

int db_buy_now(int auction_id, int user_id) {
    sqlite3_exec(g_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    
    char *sql = "SELECT buy_now_price, seller_id, status FROM auctions WHERE auction_id = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, auction_id);
    
    double buy_now_price = 0;
    int seller_id = -1;
    char status[20];
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        buy_now_price = sqlite3_column_double(stmt, 0);
        seller_id = sqlite3_column_int(stmt, 1);
        strcpy(status, (const char*)sqlite3_column_text(stmt, 2));
    } else {
        sqlite3_finalize(stmt);
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    sqlite3_finalize(stmt);
    
    if (strcmp(status, "active") != 0) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    
    if (buy_now_price <= 0) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -2;
    }
    
    sql = "SELECT balance FROM users WHERE user_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    
    double balance = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        balance = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    
    if (balance < buy_now_price) {
        sqlite3_exec(g_db, "ROLLBACK;", NULL, NULL, NULL);
        return -3;
    }
    
    sql = "UPDATE auctions SET status = 'ended', winner_id = ?, win_method = 'buy_now' WHERE auction_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, auction_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sql = "UPDATE users SET balance = balance - ? WHERE user_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, buy_now_price);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sql = "UPDATE users SET balance = balance + ? WHERE user_id = ?";
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, buy_now_price);
    sqlite3_bind_int(stmt, 2, seller_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    sqlite3_exec(g_db, "COMMIT;", NULL, NULL, NULL);
    return 0;
}

int db_get_bid_history(int auction_id, Bid bids[], int max_count) {
    char *sql = "SELECT * FROM bids WHERE auction_id = ? ORDER BY bid_time DESC LIMIT ?";
    sqlite3_stmt *stmt;
    
    sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, auction_id);
    sqlite3_bind_int(stmt, 2, max_count);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        bids[count].bid_id = sqlite3_column_int(stmt, 0);
        bids[count].auction_id = sqlite3_column_int(stmt, 1);
        bids[count].user_id = sqlite3_column_int(stmt, 2);
        bids[count].bid_amount = sqlite3_column_double(stmt, 3);
        bids[count].bid_time = sqlite3_column_int64(stmt, 4);
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}



int db_get_room_creator(int room_id) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT created_by FROM rooms WHERE room_id = ?";
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, room_id);
    
    int creator_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        creator_id = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return creator_id;
}

int db_get_auction_with_room(int auction_id, Auction *auction, int *room_id) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT auction_id, seller_id, room_id, title, description, "
                     "start_price, current_price, buy_now_price, min_increment, "
                     "current_bidder_id, start_time, end_time, duration, total_bids, "
                     "status FROM auctions WHERE auction_id = ?";
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, auction_id);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auction->auction_id = sqlite3_column_int(stmt, 0);
        auction->seller_id = sqlite3_column_int(stmt, 1);
        *room_id = sqlite3_column_int(stmt, 2);
        strncpy(auction->title, (const char*)sqlite3_column_text(stmt, 3), sizeof(auction->title)-1);
        strncpy(auction->description, (const char*)sqlite3_column_text(stmt, 4), sizeof(auction->description)-1);
        auction->start_price = sqlite3_column_double(stmt, 5);
        auction->current_price = sqlite3_column_double(stmt, 6);
        auction->buy_now_price = sqlite3_column_double(stmt, 7);
        auction->min_increment = sqlite3_column_double(stmt, 8);
        auction->current_bidder_id = sqlite3_column_int(stmt, 9);
        auction->start_time = sqlite3_column_int(stmt, 10);
        auction->end_time = sqlite3_column_int(stmt, 11);
        auction->duration = sqlite3_column_int(stmt, 12);
        auction->total_bids = sqlite3_column_int(stmt, 13);
        strncpy(auction->status, (const char*)sqlite3_column_text(stmt, 14), sizeof(auction->status)-1);
        result = 0;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

int db_get_auction_seller_and_room(int auction_id, int *seller_id, int *room_id) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT seller_id, room_id FROM auctions WHERE auction_id = ?";
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, auction_id);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *seller_id = sqlite3_column_int(stmt, 0);
        *room_id = sqlite3_column_int(stmt, 1);
        result = 0;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

int db_get_full_auction_details(int auction_id, Auction *auction, char *seller_name, char *room_name) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT a.auction_id, a.seller_id, a.room_id, a.title, a.description, "
                     "a.start_price, a.current_price, a.buy_now_price, a.min_increment, "
                     "a.current_bidder_id, a.start_time, a.end_time, a.duration, a.total_bids, "
                     "a.status, u.username, r.room_name "
                     "FROM auctions a "
                     "JOIN users u ON a.seller_id = u.user_id "
                     "JOIN rooms r ON a.room_id = r.room_id "
                     "WHERE a.auction_id = ?";
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, auction_id);
    
    int result = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auction->auction_id = sqlite3_column_int(stmt, 0);
        auction->seller_id = sqlite3_column_int(stmt, 1);
        auction->room_id = sqlite3_column_int(stmt, 2);
        strncpy(auction->title, (const char*)sqlite3_column_text(stmt, 3), sizeof(auction->title)-1);
        strncpy(auction->description, (const char*)sqlite3_column_text(stmt, 4), sizeof(auction->description)-1);
        auction->start_price = sqlite3_column_double(stmt, 5);
        auction->current_price = sqlite3_column_double(stmt, 6);
        auction->buy_now_price = sqlite3_column_double(stmt, 7);
        auction->min_increment = sqlite3_column_double(stmt, 8);
        auction->current_bidder_id = sqlite3_column_int(stmt, 9);
        auction->start_time = sqlite3_column_int(stmt, 10);
        auction->end_time = sqlite3_column_int(stmt, 11);
        auction->duration = sqlite3_column_int(stmt, 12);
        auction->total_bids = sqlite3_column_int(stmt, 13);
        strncpy(auction->status, (const char*)sqlite3_column_text(stmt, 14), sizeof(auction->status)-1);
        strncpy(seller_name, (const char*)sqlite3_column_text(stmt, 15), 49);
        strncpy(room_name, (const char*)sqlite3_column_text(stmt, 16), 99);
        result = 0;
    }
    
    sqlite3_finalize(stmt);
    return result;
}

int db_check_auction_expired(int auction_id) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT end_time FROM auctions WHERE auction_id = ? AND status = 'active'";
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, auction_id);
    
    int expired = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        time_t end_time = sqlite3_column_int(stmt, 0);
        time_t now = time(NULL);
        expired = (now >= end_time);
    }
    
    sqlite3_finalize(stmt);
    return expired;
}

int db_end_auction(int auction_id, int winner_id, const char *method) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE auctions SET status = 'ended', winner_id = ?, win_method = ? WHERE auction_id = ?";
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, winner_id);
    sqlite3_bind_text(stmt, 2, method, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, auction_id);
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (result == SQLITE_DONE) ? 0 : -1;
}

int db_get_all_active_auctions(Auction *auctions, int max_count) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT auction_id, seller_id, room_id, title, description, "
                     "start_price, current_price, buy_now_price, min_increment, "
                     "current_bidder_id, start_time, end_time, duration, total_bids, "
                     "status FROM auctions WHERE status = 'active' LIMIT ?";
    
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    
    sqlite3_bind_int(stmt, 1, max_count);
    
    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        auctions[count].auction_id = sqlite3_column_int(stmt, 0);
        auctions[count].seller_id = sqlite3_column_int(stmt, 1);
        auctions[count].room_id = sqlite3_column_int(stmt, 2);
        strncpy(auctions[count].title, (const char*)sqlite3_column_text(stmt, 3), sizeof(auctions[count].title)-1);
        strncpy(auctions[count].description, (const char*)sqlite3_column_text(stmt, 4), sizeof(auctions[count].description)-1);
        auctions[count].start_price = sqlite3_column_double(stmt, 5);
        auctions[count].current_price = sqlite3_column_double(stmt, 6);
        auctions[count].buy_now_price = sqlite3_column_double(stmt, 7);
        auctions[count].min_increment = sqlite3_column_double(stmt, 8);
        auctions[count].current_bidder_id = sqlite3_column_int(stmt, 9);
        auctions[count].start_time = sqlite3_column_int(stmt, 10);
        auctions[count].end_time = sqlite3_column_int(stmt, 11);
        auctions[count].duration = sqlite3_column_int(stmt, 12);
        auctions[count].total_bids = sqlite3_column_int(stmt, 13);
        strncpy(auctions[count].status, (const char*)sqlite3_column_text(stmt, 14), sizeof(auctions[count].status)-1);
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}
