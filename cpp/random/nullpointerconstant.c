#include <stdio.h>
#include <stdbool.h>

int main(void) {
	int *a = (int *)0; // null pointer
	int *b = NULL; // null pointer
	int *c = 0; // null pointer
	void *d = NULL; // null pointer
	void *e = (void *)0; // null pointer

	int value = 11;
	int *normalpt = &value;

	printf("Compare the pointers: \n");
	printf("Normalpt pointer %p of value %d \n", normalpt, *normalpt);
	printf("Pointers from a to e:  %p, %p, %p, %p, %p \n", a, b, c, d, e);
	// This would fail as I'm referencing void: 
	// printf("Values from a to e:  %d, %d, %d, %d, %d \n", *a, *b, *c, *d, *e);
	printf("Values from a to c:  %p, %p, %p \n", a, b, c);

	printf("Comparison between normal and a-e: \n");
	printf("a: %s\n", (a == normalpt) ? "true" : "false");
	printf("b: %s\n", (b == normalpt) ? "true" : "false");
	printf("c: %s\n", (c == normalpt) ? "true" : "false");
	printf("d: %s\n", (d == normalpt) ? "true" : "false");
	printf("e: %s\n", (e == normalpt) ? "true" : "false");
	// these will be false

	printf("Compare null pointers: \n");
	printf("%s\n", (a == b) ? "true" : "false");
	printf("%s\n", (a == c) ? "true" : "false");
	printf("%s\n", (a == d) ? "true" : "false");
	printf("%s\n", (a == e) ? "true" : "false");
	printf("%s\n", (b == c) ? "true" : "false");
	printf("%s\n", (b == d) ? "true" : "false");
	printf("%s\n", (b == e) ? "true" : "false");
	printf("%s\n", (c == d) ? "true" : "false");
	printf("%s\n", (c == e) ? "true" : "false");
	printf("%s\n", (d == e) ? "true" : "false");
	// these will be true

	return 0;
}


