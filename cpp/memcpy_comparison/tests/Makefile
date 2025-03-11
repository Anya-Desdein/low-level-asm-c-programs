SHELL = /bin/sh

.PHONY: run runt clean cleant link

CC := gcc

FLAGS = -O1 -g3 -ggdb -fno-strict-aliasing -fno-tree-dce -march=native

SRC = tests
TST = self_tests

LIBDIR = ./../implementations/

all: run

$(SRC): $(SRC).c
	$(CC) $(SRC).c -o $(SRC) $(FLAGS)

run: $(SRC)
	./$(SRC)

clean:
	rm -f tests

runt: $(TST) link
	./$(TST)
	

$(TST): $(TST).c
	$(CC) $(TST).c -o $(TST) $(FLAGS)

link:
	ls -l $(LIBDIR)*.so
	@find $(LIBDIR) -name "*.so" -exec ln -sf {} ./ \;

cleant:
	rm -f *.so
	rm -f self_tests
