SHELL = /bin/sh

.PHONY : clean

CC := gcc

NAME := simple_memcpy

FLAGS = -O3 -shared -march=raptorlake -mtune=raptorlake -fomit-frame-pointer 

all : $(NAME).so

$(NAME).so : $(NAME).c
	$(CC) $(NAME).c -o $(NAME).so $(FLAGS) 

clean : $(NAME).so
	rm -f $(NAME).so
