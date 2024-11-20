CC = gcc
CFLAGS = -Wall -Wextra -std=c11
BIN_DIR = bin

all: server client

server:
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) server.c -o $(BIN_DIR)/server

client:
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) client.c -o $(BIN_DIR)/client

clean:
	rm -rf $(BIN_DIR)
