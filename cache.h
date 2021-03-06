#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"
#include <limits.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// struct for (web) object
struct obj_t{
	ssize_t size;
	char content[MAX_OBJECT_SIZE];
};
typedef struct obj_t Obj;

// struct of cache block
struct cacheblock_t{
	int valid;
	int time;
	char req[MAXLINE]; // identifier of object
	Obj obj;
};
typedef struct cacheblock_t CB;

// struct of cache
struct cache_t{
	sem_t canwrite, mutex, timelock;
	int readcnt;
	int curtime;      // access time
	CB* block;
};
typedef struct cache_t Cache;

// find obj with identifier req in cache cch, return -1 if not found
int find_in_cache(Cache *cch, char *req, Obj *obj);

// add obj with identifier req to cache cch, return -1 if fail 
int add_to_cache(Cache *cch, char *req, Obj *obj);

// obj constructor and destructor
Obj *new_obj();
int del_obj(Obj *obj);

// cache constructor, destructor and initializer
Cache *new_cache();
int del_cache(Cache *cch);
void init_cache(Cache *cch);

// obj initializer: will set the obj size to be 0
void init_obj(Obj *obj);

// concatenate a string of char with length len to obj
// return -1 if the total length exceeds MAX_OBJECT_SIZE
int cat_obj(Obj *obj, char *buf, ssize_t len);

// copy obj src to obj dst
void copy_obj(Obj *dst, Obj *src);

void print(Obj *obj);

#endif
