    -- Users table
    CREATE TABLE IF NOT EXISTS users (
        user_id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        password TEXT NOT NULL,
        balance REAL DEFAULT 50000000.0,
        is_active INTEGER DEFAULT 1,
        created_at INTEGER DEFAULT (strftime('%s', 'now'))
    );

    -- Rooms table
    CREATE TABLE IF NOT EXISTS rooms (
        room_id INTEGER PRIMARY KEY AUTOINCREMENT,
        room_name TEXT NOT NULL,
        description TEXT,
        created_by INTEGER NOT NULL,
        max_participants INTEGER DEFAULT 10,
        current_participants INTEGER DEFAULT 0,
        total_auctions INTEGER DEFAULT 0,
        created_at INTEGER DEFAULT (strftime('%s', 'now')),
        end_time INTEGER,
        status TEXT DEFAULT 'active',
        FOREIGN KEY (created_by) REFERENCES users(user_id)
    );

    -- Auctions table
    CREATE TABLE IF NOT EXISTS auctions (
        auction_id INTEGER PRIMARY KEY AUTOINCREMENT,
        seller_id INTEGER NOT NULL,
        room_id INTEGER NOT NULL,
        title TEXT NOT NULL,
        description TEXT,
        start_price REAL NOT NULL,
        current_price REAL NOT NULL,
        buy_now_price REAL,
        min_increment REAL DEFAULT 10000.0,
        current_bidder_id INTEGER,
        start_time INTEGER,
        end_time INTEGER,
        duration INTEGER,
        total_bids INTEGER DEFAULT 0,
        status TEXT DEFAULT 'waiting',
        winner_id INTEGER,
        win_method TEXT,
        FOREIGN KEY (seller_id) REFERENCES users(user_id),
        FOREIGN KEY (room_id) REFERENCES rooms(room_id)
    );

    -- Bids table
    CREATE TABLE IF NOT EXISTS bids (
        bid_id INTEGER PRIMARY KEY AUTOINCREMENT,
        auction_id INTEGER NOT NULL,
        user_id INTEGER NOT NULL,
        bid_amount REAL NOT NULL,
        bid_time INTEGER DEFAULT (strftime('%s', 'now')),
        FOREIGN KEY (auction_id) REFERENCES auctions(auction_id),
        FOREIGN KEY (user_id) REFERENCES users(user_id)
    );

    -- User-Room relationship
    CREATE TABLE IF NOT EXISTS user_rooms (
        user_id INTEGER NOT NULL,
        room_id INTEGER NOT NULL,
        joined_at INTEGER DEFAULT (strftime('%s', 'now')),
        PRIMARY KEY (user_id, room_id),
        FOREIGN KEY (user_id) REFERENCES users(user_id),
        FOREIGN KEY (room_id) REFERENCES rooms(room_id)
    );

    -- Indexes
    CREATE INDEX IF NOT EXISTS idx_auctions_room ON auctions(room_id);
    CREATE INDEX IF NOT EXISTS idx_auctions_seller ON auctions(seller_id);
    CREATE INDEX IF NOT EXISTS idx_auctions_status ON auctions(status);
    CREATE INDEX IF NOT EXISTS idx_bids_auction ON bids(auction_id);
    CREATE INDEX IF NOT EXISTS idx_bids_user ON bids(user_id);

    -- Insert test data
    INSERT OR IGNORE INTO users (user_id, username, password, balance) VALUES 
    (1, 'alice', 'password123', 50000000.0),
    (2, 'bob', 'password123', 50000000.0),
    (3, 'charlie', 'password123', 50000000.0);
