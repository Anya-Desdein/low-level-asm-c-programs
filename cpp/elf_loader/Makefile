S$HELL = /bin/sh

.PHONY: load

.SUFFIXES:
CC := gcc
CG := clang

# Assembly
INT = example.s
ATT = example_att.s
OBJ = example.o

# Loader
LOD = elf_loader.c


all: load

### TEST FILES
# Non-PIE
example: $(OBJ)
	ld -o example example.o
$(OBJ): $(INT)
	nasm -f elf64 example.s -o example.o	

example_att: $(ATT)
	$(CC) -no-pie -nostartfiles -Wl,--no-dynamic-linker -g -o $@ $<


# PIE
example_pie: $(ATT)
	$(CC) -nostartfiles -Wl,--no-dynamic-linker -g -o $@ $<

### LOADER BUILD
elf_loader: $(LOD) 
	$(CG) $(LOD) -o elf_loader -O0 -g

### LOAD
load: elf_loader example example_att example_pie
	./elf_loader example
	./elf_loader example_att
	./elf_loader example_pie

# Clean
clean:
	rm -f elf_loader example example_att example_pie example.o
