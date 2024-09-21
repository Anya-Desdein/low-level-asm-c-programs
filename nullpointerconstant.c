#include <stdio.h>
#include <stdbool.h>

int main() {
	int *a = (int *)0;
	int *b = NULL; // null pointer
	int *c = 0; // point to 0 null pointer constant
	void *d = NULL; // probably a null pointer constant
	void *e = (void *)0; // point to null pointer constant

	int value = 11;
	int *normalpt = &value;

	printf("Compare the pointers: \n");
	printf("Normalpt: %p \n", normalpt);
	printf("From a to e:  %p, %p, %p, %p, %p \n", a, b, c, d, e);


	printf("Comparison between them: \n");
	printf("a: %s\n", (a == normalpt) ? "true" : "false");
	printf("b: %s\n", (b == normalpt) ? "true" : "false");
	printf("c: %s\n", (c == normalpt) ? "true" : "false");
	printf("d: %s\n", (d == normalpt) ? "true" : "false");
	printf("e: %s\n", (e == normalpt) ? "true" : "false");


	printf("Compare null pointers: %s\n", (a == b) ? "true" : "false");
	printf("%s\n", (a == c) ? "true" : "false");
	printf("%s\n", (a == d) ? "true" : "false");
	printf("%s\n", (a == e) ? "true" : "false");
	printf("%s\n", (b == c) ? "true" : "false");
	printf("%s\n", (b == d) ? "true" : "false");
	printf("%s\n", (b == e) ? "true" : "false");
	printf("%s\n", (c == d) ? "true" : "false");
	printf("%s\n", (c == e) ? "true" : "false");
	printf("%s\n", (d == e) ? "true" : "false");
	return 0;
}


