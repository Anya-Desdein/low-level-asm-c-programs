SHELL = /bin/sh

.PHONY: run clean

CC := gcc

FLAGS = -O1 -fno-strict-aliasing -fno-tree-dcei -march=native

SRC = custom_memcpy

all : run

$(SRC): $(SRC).c
	$(CC) $(SRC).c -o $(SRC)

run: $(SRC)
	./$(SRC)

clean:
	rm -f custom_memcpy
