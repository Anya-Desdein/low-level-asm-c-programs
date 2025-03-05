SHELL = /bin/sh

CC := gcc

SRC = main.o extern_is_ascii.o extern_cleanup.o extern_append.o

OBJ = $(SRC:.c=.o)

EXT = extern_is_ascii.o extern_cleanup.o extern_append.o

all: executable executable2 executable3

# Object creation
%.o: %.c
	$(CC) -c $< -o $@

# Static linking of libraries
add_ons.a: $(EXT)
	ar rcs add_ons.a $(EXT)

# Dynamic linking of libraries
add_ons.so: $(EXT)
	$(CC) -o add_ons.so $(EXT) -shared

executable: $(OBJ)
	$(CC) -o executable $(OBJ)

executable2: main.o add_ons.a
	$(CC) main.o add_ons.a -o executable2


executable3: main.o add_ons.so
	$(CC) -o executable3 main.o add_ons.so
	
# Clean
clean:
	rm -f *.o add_ons.a add_ons.so executable executable2 executable3
