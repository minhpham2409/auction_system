# 🚀 QUICK START GUIDE

## 📥 FILE ĐÃ TẢI VỀ

**File:** auction-modular.zip (19 KB)  
**Chứa:** Server hoàn chỉnh với SQLite database  
**Cấu trúc:** Modular, dễ maintain  

---

## ⚡ CHẠY TRONG 3 BƯỚC

### **Bước 1: Giải nén**

```bash
unzip auction-modular.zip
cd auction-modular
```

### **Bước 2: Setup**

```bash
make setup
```

**Lệnh này sẽ:**
- ✅ Xóa file cũ (nếu có)
- ✅ Tạo database (auction.db)
- ✅ Thêm 3 test users (alice, bob, charlie)
- ✅ Compile server

### **Bước 3: Chạy**

```bash
make run
```

**Output:**
```
==============================================
   ONLINE AUCTION SYSTEM - MODULAR VERSION
==============================================

[INFO] Database opened: auction.db
[INFO] Server listening on port 8080
[INFO] Database: auction.db
[INFO] Waiting for connections...
```

**✅ Server đang chạy!**

---

## 🧪 TEST

### **Test với telnet:**

```bash
# Terminal 1: Server
make run

# Terminal 2: Client
telnet localhost 8080

# Login
LOGIN|alice|password123

# Kết quả:
LOGIN_SUCCESS|1|alice|50000000.00

# List rooms
LIST_ROOMS|

# Create room
CREATE_ROOM|1|Test Room|My first room|10|3600

# Tạo auction
CREATE_AUCTION|1|1|iPhone 15 Pro|Brand new|15000000|18000000|100000|1800
```

---

## 📂 CẤU TRÚC FILE

```
auction-modular/
│
├── shared/              ← Shared headers
│   ├── types.h         
│   └── config.h        
│
├── server/             ← Server code
│   ├── main.c          ← Entry point
│   ├── database.c/h    ← SQLite layer
│   ├── handlers.c/h    ← Request handlers
│   └── network.c/h     ← Network layer
│
├── schema.sql          ← Database schema
├── Makefile            ← Build script
└── README.md           ← Documentation
```

---

## 🎯 TEST ACCOUNTS

| Username | Password | Balance |
|----------|----------|---------|
| alice | password123 | 50,000,000 VND |
| bob | password123 | 50,000,000 VND |
| charlie | password123 | 50,000,000 VND |

---

## 🔧 AVAILABLE COMMANDS

```bash
make          # Build server
make init-db  # Reset database
make setup    # Clean + Init + Build
make run      # Start server
make clean    # Clean everything
make help     # Show help
```

---

## 📊 XEM DATABASE

```bash
sqlite3 auction.db

# List all users
SELECT * FROM users;

# List all rooms
SELECT * FROM rooms;

# List all auctions
SELECT * FROM auctions;

# Exit
.quit
```

---

## 🐛 TROUBLESHOOTING

### **Lỗi: "Cannot open database"**

```bash
make init-db
```

### **Lỗi: "Address already in use"**

```bash
pkill -9 server
make run
```

### **Lỗi: "sqlite3.h not found"**

```bash
sudo pacman -S sqlite
```

---

## 📡 PROTOCOL

### **Commands:**

```
REGISTER|username|password
LOGIN|username|password
CREATE_ROOM|user_id|name|desc|max|duration
LIST_ROOMS|
JOIN_ROOM|user_id|room_id
LEAVE_ROOM|user_id
CREATE_AUCTION|user_id|room_id|title|desc|price|buy_now|increment|duration
LIST_AUCTIONS|room_id
SEARCH_AUCTIONS|keyword|min|max|min_time|max_time|status|room
PLACE_BID|auction_id|user_id|amount
BUY_NOW|auction_id|user_id
```

### **Responses:**

```
REGISTER_SUCCESS|user_id|username
LOGIN_SUCCESS|user_id|username|balance
ROOM_LIST|id;name;creator;current;max|...
AUCTION_LIST|id;title;price;buy_now;time;bids;status|...
BID_SUCCESS|auction_id|amount|total_bids|time_left
```

---

## ✨ FEATURES

✅ User management (Register, Login)  
✅ Room management (Create, Join, Leave)  
✅ Auction CRUD  
✅ Bidding system  
✅ Search with filters  
✅ SQLite database  
✅ Thread-safe  
✅ Multi-client support  

---

## 🎓 CODE STRUCTURE

### **Modular Design:**

```
database.c   → All database operations
handlers.c   → Request processing
network.c    → Client management
main.c       → Entry point
```

### **Clean Separation:**

- **Data Layer:** database.c
- **Logic Layer:** handlers.c
- **Network Layer:** network.c

---

## 🔍 EXAMPLE USAGE

### **1. Create User**

```bash
telnet localhost 8080
REGISTER|testuser|mypassword
```

**Response:**
```
REGISTER_SUCCESS|4|testuser
```

### **2. Login**

```
LOGIN|testuser|mypassword
```

**Response:**
```
LOGIN_SUCCESS|4|testuser|50000000.00
```

### **3. Create Room**

```
CREATE_ROOM|4|Gaming Room|For gaming items|20|7200
```

**Response:**
```
CREATE_ROOM_SUCCESS|1|Gaming Room
```

### **4. Create Auction**

```
CREATE_AUCTION|4|1|PS5 Console|New|10000000|12000000|100000|3600
```

**Response:**
```
CREATE_AUCTION_SUCCESS|1|PS5 Console
```

---

## 💡 NEXT STEPS

**Hiện tại có:**
- ✅ Server hoàn chỉnh
- ✅ SQLite database
- ✅ Modular structure
- ✅ All features working

**Cần thêm:**
- ⏳ Client TUI (from old client.c)
- ⏳ Web GUI (optional)
- ⏳ Real-time notifications
- ⏳ Auction timeout checker

---

## 📞 SUPPORT

**Nếu gặp lỗi:**

1. Check `make help`
2. Try `make clean && make setup`
3. Check database: `sqlite3 auction.db`
4. Check logs in terminal

---

## 🎉 DONE!

**Server đã sẵn sàng!**

```bash
cd auction-modular
make setup && make run
```

**🚀 Happy coding!**
