SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .c .o .a

SRC = main.o extern_is_ascii.o extern_cleanup.o extern_append.o

OBJ = $(SRC:.c=.o)

EXT = extern_is_ascii.o extern_cleanup.o extern_append.o

all: executable executable2

# Object creation
%.o: %.c
	gcc -c $< -o $@

# Static linking of libraries
add_ons.a: $(EXT)
	ar rcs add_ons.a $(EXT)

executable2: main.c add_ons.a
	cc main.c add_ons.a -o executable2


# Dynamic linking of libraries 
executable: $(OBJ)
	gcc -o executable $(OBJ)
	
# Clean
clean:
	rm -f *.o add_ons.a executable executable2
