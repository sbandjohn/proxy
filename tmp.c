#include "csapp.h"
#include "cache.h"

char req1[100] = "abcdefg";
char req2[100] = "xxxxxxx";
char req3[100] = "yyyy";

char *req[3];

Cache *cch;

void *readthread(void *argp){
	Pthread_detach(Pthread_self());
	char *req = (char *)argp;
	Obj *obj = new_obj();
	printf("read: %d %s\n", (int)Pthread_self(), req);
	int hit = find_in_cache(cch, req, obj);
	if (!hit){
		printf("%s miss\n", req);
	}else{
		printf("%s hit:(", req);
		print(obj);
		printf(")\n");
	}
	del_obj(obj);
	return NULL;
}

void *writethread(void *argp){
	Pthread_detach(Pthread_self());
	char *req = (char *)argp;
	Obj *obj = new_obj();
	char buf[MAXLINE];
	sprintf(buf, "write: %s %d", req, (int)pthread_self());
	cat_obj(obj, buf, strlen(buf));
	add_to_cache(cch, req, obj);
	del_obj(obj);
	return NULL;
}


int main(int argc, char *argv[]){
	cch = new_cache();
	req[0] = req1;
	req[1] = req2;
	req[2] = req3;

	pthread_t tid;
	int n = 10000;
	printf("%p\n", cch);
	for (int i = 0; i<n; ++i){
		if (i%3 == 0)
			Pthread_create(&tid, NULL, writethread, req[i%3]);
		else
			Pthread_create(&tid, NULL, readthread, req[i%3]);
	}
	while (1);
	del_cache(cch);
}
 
