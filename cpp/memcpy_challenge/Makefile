SHELL = /bin/sh

.PHONY: clean

CC := gcc

FLAGS = -O3 -shared -march=native

all : mcpc.so

mcpc.so : mcpc.c
	$(CC) mcpc.c -o mcpc.so $(FLAGS) 

clean : mcpc.so
	rm -f mcpc.so
