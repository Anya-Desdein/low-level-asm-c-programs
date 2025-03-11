SHELL = /bin/sh

.PHONY : clean

CC := gcc

NAMES := cmemcpy cmemcpy2

FLAGS = -O3 -shared -fPIC -march=raptorlake -mtune=raptorlake -fomit-frame-pointer -fkeep-inline-functions -mno-avx 

all : $(NAMES:%=%.so)

%.so : %.c
	$(CC) $< -o $@ $(FLAGS) 

clean :
	rm -f $(NAMES:%=%.so)
