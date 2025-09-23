# Trình biên dịch
CC = gcc

# Cờ biên dịch
CFLAGS = -Wall -Wextra -g -pthread

# Thư viện liên kết
LIBS = -lsctp

# File nguồn
SERVER_SRC = p1-256.c
CLIENT_SRC = l1-256.c

# Thư mục build
BUILD_DIR = build

# File thực thi (đặt trong build/)
SERVER_BIN = $(BUILD_DIR)/sctp_x2ap_server
CLIENT_BIN = $(BUILD_DIR)/sctp_x2ap_client

# Quy tắc mặc định
all: $(SERVER_BIN) $(CLIENT_BIN)

# Tạo thư mục build nếu chưa có
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build server
$(SERVER_BIN): $(SERVER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# Build client
$(CLIENT_BIN): $(CLIENT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

# Run server
runp: $(SERVER_BIN)
	@echo "---- Running SCTP Server ----"
	./$(SERVER_BIN)

# Run client
runl: $(CLIENT_BIN)
	@echo "---- Running SCTP Client ----"
	./$(CLIENT_BIN)

# Clean tất cả file biên dịch
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean runp runl
