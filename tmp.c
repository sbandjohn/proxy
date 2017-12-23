#include "csapp.h"
#include "cache.h"

int main(int argc, char *argv[]){
	Cache *cch = new_cache();
	Obj *obj = new_obj();
	for (int i = 0; i<argc; ++i)
		cat_obj(obj, argv[i], strlen(argv[i]));
	print(obj);
	printf("\n");
	init_cache(cch); // make gcc happy 
	exit(0);
}
 
