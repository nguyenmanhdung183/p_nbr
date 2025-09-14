CC = gcc
CFLAGS = -Wall -O2
LIBS = -lpthread -lsctp

SRC = node.c
BIN = node

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $(BIN) $(SRC) $(LIBS)

run1: $(BIN)
	./$(BIN) 5000 127.0.0.1 5001

run2: $(BIN)
	./$(BIN) 5001 127.0.0.1 5000

clean:
	rm -f $(BIN)
