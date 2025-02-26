SHELL = /bin/sh

.PHONY: load

### Disable implicit rules
.SUFFIXES:
##########################

CC := clang

# Common AT&T compilation flags
FLAGS = -nostartfiles -Wl,--no-dynamic-linker -g -o 

# Test assembly files
NASM = example
ATT = example_att
PIE = example_pie

# ELF compilation flags
ELF_FLAGS = -g3 -ggdb -fsanitize=address

# Loader
LOD = elf_loader


all: load

### Generate ELFs for Testing

# Nasm non-PIE
$(NASM): $(NASM).o
	ld -o $(NASM) $(NASM).o
$(NASM).o: $(NASM).s
	nasm -f elf64 $(NASM).s -o $(NASM).o	

# AT&T non-PIE
$(ATT): $(ATT).s
	$(CC) -no-pie $(FLAGS) $@ $<


# AT&T PIE
$(PIE): $(ATT).s
	$(CC) $(FLAGS) $@ $<

### ELF LOADER BUILD
$(LOD): $(LOD).c 
	$(CC) $(LOD).c -o $(LOD) $(ELF_FLAGS)

### LOAD
load: $(LOD) $(NASM) $(ATT) $(PIE)
	./$(LOD) $(NASM)
	./$(LOD) $(ATT)
	./$(LOD) $(PIE)

# Clean
clean:
	rm -f $(LOD) $(ATT) $(PIE) $(NASM) $(NASM).o
