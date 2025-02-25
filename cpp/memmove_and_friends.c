#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int mult(int x, int y) {
	return x * y;
}

int counter() {
	static int count = 0;
	count++;
	printf("%d\n", count);
}

int print_str(int str, int size) {
	if (!str) {
		printf("NULL string\n");
		return 1;
	}

	if (!size) {
		printf("NULL size\n");
		return 1;	
	}

	printf("Value: %d\n", str);
	char buffer[12];
	sprintf(buffer, "%d", str);
	puts("===============================");
	for (int i=0; i<sizeof(buffer); i++) {
		if (buffer[i] < 48)
			continue;

		printf("Digit %c in ASCII: %d\n", buffer[i], buffer[i]);
	}
	puts("===============================");
	
	return 0;
}

int main() {

	int (*ptr1)(int, int);
	ptr1 = &mult;

	int a = 10, b = 20;
	int  m = ptr1(a,b);
	size_t sz = sizeof(m);

	int checker = print_str(m,(int)sz);
	if (checker == 1) {
		return 1;
	}

	// sizeof return type is size_t
	printf("Size of TYPE int: %zu\n", sizeof(int));
	printf("Size of TYPE unsigned int: %zu\n", sizeof(unsigned int));
	printf("Size of TYPE char: %zu\n", sizeof(char));
	printf("Size of TYPE char*: %zu\n", sizeof(char*));
	printf("Size of TYPE function*: %zu\n", sizeof(ptr1));
	printf("Size of TYPE unsigned long: %zu\n", sizeof(unsigned long));
	printf("Size of TYPE unsigned long long: %zu\n", sizeof(unsigned long long));
	printf("Size of TYPE signed long long: %zu\n", sizeof(signed long long));
	printf("Size of TYPE unsigned short: %zu\n", sizeof(unsigned short));
	
	puts("");
	
	// Array
	int arr[11] = {1,2,3,111,18};
	printf("Size of array[11]: %zu\n", sizeof(arr));
	char arr2[3] = "UwU";
	printf("Size of data types in array: %zu\n", strlen(arr2));
	
	// Int
	int c = 11;
	printf("Size of int c: %zu\n", sizeof(c));

	// Sign-extend
	int8_t z = -6;
	int16_t z1 = (int16_t)z;
	
	//Zero-extend
	uint16_t z2 = (uint16_t)(uint8_t)z;

	printf("Sign-extend: %hd,  Zero-extend: %hx\n", z1, z2);

	puts("");
	
	// Auto pointer decay
	int *integers_only = malloc(sizeof(*integers_only)*5);
	printf("Array sizeof: %zu\n", sizeof(*integers_only)*5);
	free(integers_only);
	integers_only = NULL;
	
	puts("");
	printf("Static int inside fn will retain value between fn calls:\n");
	counter();
	counter();
	counter();
	
	puts("");

	char strr[] = "Copy me";
	printf("%s\n", strr);

	puts("Memmove: void* dest, const void* source, size_t");
	memmove(strr+1, strr+2, 1);
	printf("%s\n", strr);

	puts("Memcpy: void* dest, const void* source, size_t");
	memcpy(strr+1, strr+4, 1);
	printf("%s\n", strr);
	

	puts("");

	// It should prevent overlapping the src with sth else
	puts("Memmove: void* dest, const void* source, size_t");
	memmove(strr+3, strr+1, 3);
	printf("%s\n", strr);

	puts("Memcpy: void* dest, const void* source, size_t");
	memcpy(strr+3, strr+1, 3);
	printf("%s\n", strr);
	

	puts("");


	char strrr[10];
	memset(strrr, 'A', 10);
	memset(strrr+2, 'o', 1);
	printf("%s\n", strrr);

	return 0;
}

