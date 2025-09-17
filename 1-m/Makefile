CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lsctp

SERVER = x2_server
CLIENT = x2_client

all: $(SERVER) $(CLIENT)

$(SERVER): x2_sctp_server.c
	$(CC) $(CFLAGS) -o $(SERVER) x2_sctp_server.c $(LDFLAGS)

$(CLIENT): x2_sctp_client.c
	$(CC) $(CFLAGS) -o $(CLIENT) x2_sctp_client.c $(LDFLAGS)

runs: $(SERVER)
	./$(SERVER)

# Dùng: make run-client IP=1.2.3.4
runc: $(CLIENT)
	./$(CLIENT)


clean:
	rm -f $(SERVER) $(CLIENT) *.o
