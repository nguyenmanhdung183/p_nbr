# Trình biên dịch
CC = gcc

# Cờ biên dịch
CFLAGS = -Wall -g -pthread

# Thư viện liên kết
LIBS = -lsctp

# File nguồn
SERVER_SRC = sctp_x2ap_server.c
CLIENT_SRC = sctp_x2ap_client.c

# File thực thi
SERVER_BIN = sctp_x2ap_server
CLIENT_BIN = sctp_x2ap_client

# Quy tắc mặc định
all: $(SERVER_BIN) $(CLIENT_BIN)

# Build server
$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_SRC) $(LIBS)

# Build client
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_SRC) $(LIBS)

# Run server
runs: $(SERVER_BIN)
	./$(SERVER_BIN)

# Run client
runc: $(CLIENT_BIN)
	./$(CLIENT_BIN)

# Clean tất cả file biên dịch
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
