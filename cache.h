#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct cache_t{
	char content[MAX_CACHE_SIZE];
};
typedef struct cache_t Cache;

struct obj_t{
	ssize_t size;
	char content[MAX_OBJECT_SIZE];
};
typedef struct obj_t Obj;

int find_in_cache(Cache *cch, char *req, Obj *obj);
int add_to_cache(Cache *cch, Obj *ojb);

Obj *new_obj();
int del_obj(Obj *obj);

Cache *new_cache();
int del_cache(Cache *cch);

void init_cache(Cache *cch);

void init_obj(Obj *obj);
int cat_obj(Obj *obj, char *buf, ssize_t len);
void print(Obj *obj);

#endif
