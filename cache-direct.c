#include "cache.h"

static int cbnum = MAX_CACHE_SIZE / sizeof(CB);

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
	cch->block = malloc(cbnum*sizeof(CB));
	if (cch->block == NULL){
		free(cch);
		return NULL;
	}
	init_cache(cch);
	return cch;
}

int del_cache(Cache *cch){
	free(cch->block);
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
	int i;
	for (i = 0; i<len; ++i)
		obj->content[cur + i] = buf[i];
	obj->size += len;
	return 0;
}

void copy_obj(Obj *dst, Obj *src){
	memcpy(dst, src, sizeof(Obj));
}

void print(Obj *obj){
	int i;
	for (i = 0; i< obj->size; ++i)
		putchar(obj->content[i]);
}

static void init_cb(CB *cb){
	cb->valid = 0;
	strcpy(cb->req, "\0");
	init_obj(&cb->obj);
}

void init_cache(Cache *cch){
	Sem_init(&cch->canwrite, 0, 1);
	Sem_init(&cch->mutex, 0, 1);
	cch->readcnt = 0;
	int i;
	for (i = 0; i<cbnum; ++i)
		init_cb(&cch->block[i]);
}

static int get_hash(char *buf){
	int ans = 0;
	int i;
	for (i = 0; i < HASH_LEN && buf[i]; ++i)
		ans = (ans*BASE + buf[i]) % cbnum;
	if (ans < 0) ans += cbnum;
	return ans;
}

int add_to_cache(Cache *cch, char *req, Obj *obj){
	if (obj->size > MAX_OBJECT_SIZE) return -1; 
	int h = get_hash(req);
	CB *cb = &cch->block[h];

	//printf("thread %d add to cache %d:\n", (int)Pthread_self(), h);
	//printf("thread %d wait for write\n", (int)Pthread_self());
	P(&cch->canwrite);

	//printf("thread %d start write\n", (int)Pthread_self());
	cb->valid = 1;
	strcpy(cb->req, req);
	copy_obj(&cb->obj, obj);

	V(&cch->canwrite);
	//printf("thread %d write done\n", (int)Pthread_self());
	return 0;
}

int find_in_cache(Cache *cch, char *req, Obj *obj){
	int h = get_hash(req);
	printf("thread:%d  h:%d\n", (int)Pthread_self(), h);
	CB *cb = &cch->block[h];
	int hit = 0;

	//printf("thread %d wait for read\n", (int)Pthread_self());
	P(&cch->mutex);
	//printf("threal %d start reading\n", (int)Pthread_self());
	++cch->readcnt;
	if (cch->readcnt == 1)
		P(&cch->canwrite);
	V(&cch->mutex);

	// hit or miss
	if (strcmp(cb->req, req) == 0){
		copy_obj(obj, &cb->obj);
		hit = 1;
	}

	P(&cch->mutex);
	--cch->readcnt;
	if (cch->readcnt == 0)
		V(&cch->canwrite);
	V(&cch->mutex);

	//printf("thread %d read done\n", (int)Pthread_self());
	return hit;
}

