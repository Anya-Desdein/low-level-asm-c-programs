SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .c .o .a

all: executable executable2

# Object creation
extern_cleanup.o: extern_cleanup.c
	gcc -c extern_cleanup.c -o extern_cleanup.o

extern_is_ascii.o: extern_is_ascii.c
	gcc -c extern_is_ascii.c -o extern_is_ascii.o
	
extern_append.o: extern_append.c
	gcc -c extern_append.c -o extern_append.o

main.o: main.c
	gcc -c main.c -o main.o


# Static linking of libraries
add_ons.a: extern_is_ascii.o extern_append.o extern_cleanup.o
	ar rcs add_ons.a extern_is_ascii.o extern_append.o extern_cleanup.o

executable2: main.c add_ons.a
	cc main.c add_ons.a -o executable2


# Dynamic linking of libraries 
executable: main.o extern_cleanup.o extern_is_ascii.o extern_append.o
	gcc -o executable main.o extern_cleanup.o extern_is_ascii.o extern_append.o
	
# Clean
clean:
	rm -f *.o add_ons.a executable executable2
