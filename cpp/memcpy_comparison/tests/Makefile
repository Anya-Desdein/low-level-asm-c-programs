SHELL = /bin/sh

.PHONY: run runt runa clean cleant cleana link

CC := gcc

FLAGS = -O1 -g3 -ggdb -fno-strict-aliasing -fno-tree-dce -march=native
SRD_FLAGS = -shared -fPIC

SRC = tests
TST = self_tests
SRD = perf_utils

LIBDIR = ./../implementations/

all: runa

runa: $(TST) link $(SRC)
	./$(SRC)

run: $(SRC)
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

cleana:
	rm -f tests
	rm -f *.so
	rm -f self_tests

clean:
	rm -f tests

cleant:
	rm -f *.so
	rm -f self_tests
