#include <stdio.h>
#include <math.h>
#include <stdbool.h>

int main() {
	double a = NAN;
	int *b = NULL;
	int c = 0;
	float d = 0;

	printf("Let's check if a NAN, pointer to NULL, 0 int and 0 float are in fact False\n");
	printf("C23 ISO says that NAN will be true, consecutive results are presented in boolean:\n");
	printf("%d, %d, %d, %d \n", (bool)a, (bool)b, (bool)c, (bool)d);

}


