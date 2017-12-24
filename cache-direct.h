#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define HASH_LEN 256
#define BASE 233

struct obj_t{
	ssize_t size;
	char content[MAX_OBJECT_SIZE];
};
typedef struct obj_t Obj;

struct cacheblock_t{
	int valid;
	char req[MAXLINE];
	Obj obj;
};
typedef struct cacheblock_t CB;

struct cache_t{
	sem_t canwrite, mutex;
	int readcnt;
	CB* block;
};
typedef struct cache_t Cache;

int find_in_cache(Cache *cch, char *req, Obj *obj);
int add_to_cache(Cache *cch, char *req, Obj *obj);

Obj *new_obj();
int del_obj(Obj *obj);

Cache *new_cache();
int del_cache(Cache *cch);

void init_cache(Cache *cch);

void init_obj(Obj *obj);
int cat_obj(Obj *obj, char *buf, ssize_t len);
void copy_obj(Obj *dst, Obj *src);
void print(Obj *obj);


#endif
