SHELL = /bin/sh

CC := gcc

SRC = communicator
CFLAGS = -Wall -Wextra -O0 -g3 -ggdb


all: build

build:
	$(CC) $(CFLAGS) $(SRC).c -o $(SRC)

run: build 
	./$(SRC)

clean: 
	rm -f $(SRC)
