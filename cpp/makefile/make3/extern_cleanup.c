#include <stdio.h>

void clfl(FILE **ptr) {
	fclose(*ptr);
}
