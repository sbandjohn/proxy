#include "csapp.h"

int main(){
	sem_t s;
	P(&s);
	V(&s);
}

