#include <stdio.h>

int main(int argc, char **argv){
	char str1[11];
	scanf("%10s", str1);
	int no1;
	while (scanf("%d", &no1) == 1){
		if (no1 == -1){
			printf("%s\n", str1);
		}else if (0 <=  no1 && no1<=9){
			printf("%c\n", str1[no1]);
		}else{
			printf("ERROR\n");
			return 1;
		}
	}
	return 0;
}
			
