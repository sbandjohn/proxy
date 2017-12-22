#include "csapp.h"

int main(int argc, char *argv[1]){
	char s[MAXLINE], t[MAXLINE];
	sscanf(argv[1], "%[a-z]:%[a-z]\n", t, s);
	printf("%s\n%s\n",t,s);
	exit(0);
}

