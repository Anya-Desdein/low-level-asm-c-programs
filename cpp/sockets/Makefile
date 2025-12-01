SHELL = /bin/sh

CC := gcc

SRC_R = sock_respond
SRC_C = communicator

CFLAGS = -Wall -Wextra -O0 -g3 -ggdb


all: build

build:
	$(CC) $(CFLAGS) $(SRC_R).c -o $(SRC_R)
	$(CC) $(CFLAGS) $(SRC_C).c -o $(SRC_C)

run_b: build 
	./$(SRC_R)

run_c: build 
	./$(SRC_C)

clean: 
	rm -f $(SRC_R) $(SRC_C)
