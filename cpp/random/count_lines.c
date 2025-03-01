#include <stdio.h>
#include <string.h>

int main(){
	FILE *file = fopen("./lines", "r");
	if (file == NULL) {
		perror("Failed to open your fucking file");
		return 1; 
	}
	
	printf("File opened \n");
	int line_num = 0;
	char line_buffer[256];

	while (fgets(line_buffer, 256, file)) {
		line_num += 1;
	}

	fclose(file);
	printf("%d", line_num);
	printf("\n");
	return 0;
}

