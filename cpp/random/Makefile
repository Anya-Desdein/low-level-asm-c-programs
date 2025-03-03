SHELL = /bin/sh

.PHONY: clean

CC := gcc

FLAGS = -O1 -fno-strict-aliasing -fno-tree-dce

SRC = custom_memcpy

all : custom_memcpy

custom_memcpy: $(SRC).c
	$(CC) $(SRC).c -o $(SRC)

clean:
	rm -f custom_memcpy
