SHELL = /bin/sh

.PHONY: run clean symlink

CC := gcc

FLAGS = -O1 -fno-strict-aliasing -fno-tree-dce -march=native

SRC = custom_memcpy

all: run

$(SRC): $(SRC).c
	$(CC) $(SRC).c -o $(SRC) $(FLAGS)

run: $(SRC)
	./$(SRC)

clean:
	rm -f custom_memcpy

symlink:
	ln -sf ./../implementations/simple_memcpy.so ./simple_memcpy.so
