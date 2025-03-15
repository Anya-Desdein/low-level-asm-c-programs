SHELL = /bin/sh

.PHONY: build clean run link runt cleant

CC := gcc

FLAGS = -O0 -g3 -ggdb -fno-strict-aliasing -fno-tree-dce -march=native -fsanitize=address -fsanitize=undefined -Wpedantic -Wall -Wextra -std=gnu17
SRD_FLAGS = -shared -fPIC

SRC = tests
TST = self_tests
SRD = perf_utils

LIBDIR = ./../implementations/

all: build

build: $(TST) link $(SRC)
	
run: $(SRC) link
	./$(SRC)

runt: $(TST) link
	./$(TST)

$(SRC): $(SRC).c $(SRD).so
	$(CC) $(SRC).c -o $(SRC) $(FLAGS)

$(TST): $(TST).c $(SRD).so 
	$(CC) $(TST).c -o $(TST) $(FLAGS)

$(SRD).so: $(SRD).c
	$(CC) $(SRD).c -o $(SRD).so $(FLAGS) $(SRD_FLAGS)

link:
	ls -l $(LIBDIR)*.so
	@find $(LIBDIR) -name "*.so" -exec ln -sf {} ./ \;

clean:
	rm -f tests
	rm -f *.so
	rm -f self_tests

cleant:
	rm -f self_tests
