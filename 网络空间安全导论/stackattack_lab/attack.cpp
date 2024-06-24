#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void attack() {
	printf("Attack!\n");
}

void func() {
	char str[6];
	FILE *fp = fopen("./a.bin","r");
	fscanf(fp,"%s",str);
	printf("---\n");            
}

int main() {
	func();
    printf("Pass.\n");
	return 0;
}
