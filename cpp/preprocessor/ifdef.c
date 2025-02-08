#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <iso646.h>
#include <assert.h>

#define DUPA 12
#define TOT_MUSICA -1

#define MIN(a,b) ((a)>(b)?(a):(b))

int f1(unsigned long num) {
	return num*num;
}

#define F1(...) f1(__VA_ARGS__)
#define F(...) (0 __VA_OPT__(,)__VA_ARGS__)

void f2(int argc, int argv[]) {
	printf("Argument list:\n");
	for (int i=0; i < argc; i++) {
		printf(" %d\n", argv[i]);
	}
}

#define F2(...) f2((sizeof((int[]){__VA_ARGS__}) / sizeof(int)), (int[]){__VA_ARGS__})

int main(void) {
	#if defined DUPA 
		printf("if defined\n");
	#endif

	#if !defined DUPA
		printf("if !defined");
	#endif

	#if !(defined TOT_MUSICA && TOT_MUSICA > 0)
		puts("TOT MUSICA <= 0\n");
	#endif

	if (1 == 1 and 2 == 2) {
		puts("ISO646 header is required to use those funny little alternative operators like not_eq and xor");
	}

	printf("\n");
	printf("Everyone, please welcome the smol one: %d \n" , MIN(11,22));

	printf(" %d %d %d %d\n ", F1(2), F1(4), F1(8), F1(16));
	
	printf("\n");
	
	F2(2,3);

	F2(11, 21, 37);
	printf("F result: %d \n", F());

	return 0;
}
