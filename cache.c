#include "cache.h"

Obj *new_obj(){
	Obj *obj = malloc(sizeof(Obj));
	if (obj == NULL) return NULL;
	init_obj(obj);
	return obj;
}

int del_obj(Obj *obj){
	free(obj);
	return 0;
}

Cache *new_cache(){
	Cache *cch = malloc(sizeof(Cache));
	if (cch == NULL) return NULL;
	init_cache(cch);
	return cch;
}

int del_cache(Cache *cch){
	free(cch);
	return 0;
}

void init_obj(Obj *obj){
	obj->size = 0;
}

int cat_obj(Obj *obj, char *buf, ssize_t len){
	ssize_t cur = obj->size;
	ssize_t left = MAX_OBJECT_SIZE - cur;
	if (len < 0 || left < len) return -1;
	for (int i = 0; i<len; ++i)
		obj->content[cur + i] = buf[i];
	obj->size += len;
	return 0;
}

void print(Obj *obj){
	for (int i = 0; i< obj->size; ++i)
		putchar(obj->content[i]);
}

void init_cache(Cache *cch){
	printf("init cache\n");
}

