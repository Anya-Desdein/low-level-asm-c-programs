#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


#define MIN(a,b) (((a) < (b)) ? (a) : (b))

void iffy(int argc, int argv[]) {
	printf("arg count: %d\n", argc);
	printf("args:\n");
	for (int i=0; i<argc; i++) {
		printf(" %d\n", argv[i]);
	}
}

#define IFFY(...) iffy((sizeof((int[]){__VA_ARGS__ __VA_OPT__(,0,21,37)}) / sizeof(int)), (int[]){__VA_ARGS__ __VA_OPT__(,0,21,37)})

int main(void) {
	
	printf("MIN:  %d\n" , MIN(1,2));

	IFFY(1,2);
	IFFY();

	const char* 🫳 = "pat";
	printf(" 🫳 == %s\n", 🫳);

	return 0;
}
