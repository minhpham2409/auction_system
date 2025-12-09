
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I.
SERVER_LDFLAGS = -lsqlite3 -lpthread
CLIENT_LDFLAGS = -pthread

# Directories
SERVER_DIR = server
CLIENT_DIR = client
SHARED_DIR = shared

# Server files
SERVER_SRCS = $(SERVER_DIR)/main.c \
              $(SERVER_DIR)/database.c \
              $(SERVER_DIR)/handlers.c \
              $(SERVER_DIR)/network.c \
              $(SERVER_DIR)/broadcast.c \
              $(SERVER_DIR)/timer.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)
SERVER_BIN = auction_server

# Client files
CLIENT_SRCS = $(CLIENT_DIR)/main.c \
              $(CLIENT_DIR)/network.c \
              $(CLIENT_DIR)/ui.c \
              $(CLIENT_DIR)/features.c \
              $(CLIENT_DIR)/notifications.c

CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
CLIENT_BIN = auction_client

# Build all
all: $(SERVER_BIN) $(CLIENT_BIN)

# Build server
$(SERVER_BIN): $(SERVER_OBJS)
	@echo "🔨 Linking server..."
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o $(SERVER_BIN) $(SERVER_LDFLAGS)
	@echo "✅ Server built successfully!"

# Build client
$(CLIENT_BIN): $(CLIENT_OBJS)
	@echo "🔨 Linking client..."
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o $(CLIENT_BIN) $(CLIENT_LDFLAGS)
	@echo "✅ Client built successfully!"

# Compile .c to .o
%.o: %.c
	@echo "📦 Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Initialize database
init-db:
	@echo "📊 Initializing database..."
	@sqlite3 auction.db < schema.sql
	@echo "✅ Database initialized with test users!"
	@echo ""
	@echo "Test accounts:"
	@echo "  Username: alice   Password: password123"
	@echo "  Username: bob     Password: password123"
	@echo "  Username: charlie Password: password123"

# Setup (clean + init + build)
setup: clean init-db all
	@echo ""
	@echo "🎉 Setup complete!"
	@echo ""
	@echo "🚀 Quick Start:"
	@echo "  Terminal 1: make run          # Start server"
	@echo "  Terminal 2: make run-client   # Start client"

# Clean everything
clean:
	@echo "🧹 Cleaning..."
	@rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_BIN) $(CLIENT_BIN)
	@echo "✅ Cleaned!"

# Clean build files only (keep database)
clean-build:
	@echo "🧹 Cleaning build files..."
	@rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_BIN) $(CLIENT_BIN)
	@echo "✅ Build files cleaned!"

# Clean database only
clean-db:
	@echo "🧹 Cleaning database..."
	@rm -f auction.db
	@echo "✅ Database cleaned!"

# Run server
run: $(SERVER_BIN)
	@echo "🚀 Starting server..."
	./$(SERVER_BIN)

# Run client
run-client: $(CLIENT_BIN)
	@echo "🚀 Starting client..."
	./$(CLIENT_BIN)

# Help
help:
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo "  AUCTION SYSTEM v2.0 - BUILD COMMANDS"
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "📦 BUILD:"
	@echo "  make              - Build server and client"
	@echo "  make setup        - Clean + Init DB + Build all"
	@echo ""
	@echo "🗄️  DATABASE:"
	@echo "  make init-db      - Initialize database with schema"
	@echo "  make clean-db     - Remove database file"
	@echo ""
	@echo "🚀 RUN:"
	@echo "  make run          - Start server (port 8080)"
	@echo "  make run-client   - Start client"
	@echo ""
	@echo "🧹 CLEAN:"
	@echo "  make clean        - Remove all build files"
	@echo "  make clean-build  - Remove build files (keep DB)"
	@echo ""
	@echo "📚 HELP:"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
	@echo ""
	@echo "🎯 QUICK START:"
	@echo "  1. make setup"
	@echo "  2. make run          (Terminal 1)"
	@echo "  3. make run-client   (Terminal 2)"
	@echo ""

.PHONY: all clean clean-build clean-db init-db run run-client setup help
