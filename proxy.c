/*
* 林涛 1600012773
* a very simple proxy that only support HTTP request 
* use thread to implement concurrency
* cache every web object it receives from the host if the size is not too large
* use the request line and headers to identify web object for caching
* more details of cache is in cache.c
*/

#include "csapp.h"
#include "cache.h"
#include <assert.h>

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

// serve client on connfd
void serve(int connfd);

// thread running serve()
void *thread(void *argp);

// send a error page to client on fd
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg);

// struct of request line
struct reqline_t{
	char *scheme;
	char *hostname;
	char *port;
	char *filename;
};

// parse uri and store scheme, hostname, etc, into reqlinep
// return 0 if ok; -1 if fail
int parse_uri(char *uri, struct reqline_t *reqlinep);

// convert a string to a lowercase string
void to_lowercase(char *s);

// read request headers from client and
// transform them to new headers to forward to host
// return 0 if ok; -1 if fail
int generate_requesthdrs(rio_t *rp, struct reqline_t *reqp, char *res);

// fetch web object from host on myfd 
// pass it back to client on fd
// store the web object in obj
// set needcache to indicate whehter this obj should be cached
// return 0 if ok; -1 if fail
int fetch_and_pass(int fd, int myfd, Obj *obj, int *needcache);

// global cache
Cache *cch;

int main(int argc, char *argv[])
{
	printf("%s", user_agent_hdr);
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;

	assert(argc == 2);

	Signal(SIGPIPE, SIG_IGN);

	if ((cch = new_cache()) == NULL) return 0;

	listenfd = Open_listenfd(argv[1]);

	while (1){
		clientlen = sizeof(struct sockaddr_storage);
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);
		Pthread_create(&tid, NULL, thread, (void*)(long)connfd);
	}

	return 0;
}

void *thread(void *argp){
	int connfd = (int)(long)argp;
	Pthread_detach(Pthread_self());
	printf("Connected\n");
	serve(connfd);
	Close(connfd);
	printf("close connection\n");
	return NULL;  // make gcc happy
}

void serve(int fd){
	unsigned tid = Pthread_self();
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;

	printf("server %u start\n", tid);
	/* Read request line and headers */
	rio_readinitb(&rio, fd);
	ssize_t rc = rio_readlineb(&rio, buf, MAXLINE);
	if (rc <= 0)  //line:netp:doit:readrequest
		return;

	printf("server %u got request line: %s\n", tid, buf);

	if (sscanf(buf, "%s %s %s\r\n", method, uri, version) != 3){
		clienterror(fd, buf, "23333333", "Invalid Request Line",
				"Invalid request line!");
		return ;
	}

	printf("server %u got uri:(%s) version:(%s)\n", tid, uri, version);

	// check method and version
	if (strcasecmp(method, "GET")) {
		clienterror(fd, method, "501", "Not Implemented",
				"Proxy does not implement this method");
		return ;
	}
	to_lowercase(version);
	if (strcmp(version, "http/1.0") && strcmp(version, "http/1.1")){
		clienterror(fd, version, "505", "Not supported",
				"Proxy does not support this version");
		return ;
	}

	/* parse URI from GET request */
	char scheme[MAXLINE], hostname[MAXLINE], filename[MAXLINE], port[MAXLINE];
	struct reqline_t req;
	req.scheme = scheme;
	req.hostname = hostname;
	req.filename = filename;
	req.port = port;

	int err = parse_uri(uri, &req);
	printf("server %u parse: err: %d "
			"scheme:%s hostname:%s port:%s filename:%s\n",
			tid, err,
			req.scheme, req.hostname, req.port, req.filename);

	if (err == -1){
		clienterror(fd, uri, "23333", "Invalid URI",
				"Proxy cannot parse the URI");
		return ;
	}
	if (strcmp(req.scheme, "http")){
		clienterror(fd, req.scheme, "501", "Not Implemented",
				"Proxy does not implement this scheme");
		return ;
	}

	/* generate the headers to forward from the request */
	char myhdrs[MAXLINE];
	err = generate_requesthdrs(&rio, &req, myhdrs);
	if (err == -1){
		clienterror(fd, "oops", "23333", "Unable to generate headers",
				"Proxy cannot generate headers from the request");
		return ;
	}

	/* generate the full request */
	char request[MAXLINE];
	sprintf(request, "GET %s HTTP/1.0\r\n", req.filename);
	strcat(request, myhdrs);
	printf("full request:\n%s", request);

	// find the requested page in cache
	Obj *obj = new_obj();   // this obj should be deleted
	if (obj == NULL) return ;

	int hit = find_in_cache(cch, request, obj);
	
	// hit case
	if (hit){
		printf("server %u cache hit\n", tid);
		if (rio_writen(fd, obj->content, obj->size) == -1){
			printf("error while writing to client\n");
		}
	}
	// miss case
	else{
		printf("server %u cache miss\n", tid);
		printf("try to connect %s:%s\n", req.hostname, req.port);

		int myfd = open_clientfd(req.hostname, req.port);
		if (myfd < 0){
			clienterror(fd, uri, "23333", "Unable to connect host",
					"Proxy cannot connect to host");
		}else{
			printf("sending full request\n");
			err = rio_writen(myfd, request, strlen(request));
			if (err == -1){
				clienterror(fd, uri, "23333", "Write Error",
						"Proxy cannot write to server");
			}else{
				printf("sent, waiting for response\n");
				int needcache = 0;
				err = fetch_and_pass(fd, myfd, obj, &needcache);
				if (err == 0){
					if (needcache)
						add_to_cache(cch, request, obj);
				}
			}
			Close(myfd);
		}
	}

	del_obj(obj);
	printf("server %u done\n", tid);
}

void to_lowercase(char *s){
	int i;
	for (i = 0; s[i]; ++i)
		if (s[i]>='A' && s[i]<='Z') s[i] = 'a' + s[i]-'A';
}

int parse_uri(char *uri, struct reqline_t *reqlinep){
	char * send = strstr(uri, "://");
	if (send == NULL) return -1;
	*send = 0;
	strcpy(reqlinep->scheme, uri);
	to_lowercase(reqlinep->scheme);
	*send = ':';
	uri = send + 3;
	char *cur = uri;
	while (*cur && *cur != ':' && *cur!='/') ++cur;
	char tmp = *cur;
	*cur = 0;
	strcpy(reqlinep->hostname, uri);
	to_lowercase(reqlinep->hostname);
	*cur = tmp;
	if (!*cur){
		strcpy(reqlinep->port, "80");
		send = cur;
	}else if (*cur == ':'){
		send = cur + 1;
		while (*send && *send!='/') ++send;
		tmp = *send;
		*send = 0;
		strcpy(reqlinep->port, cur+1); 
		*send = tmp;
	}else if (*cur == '/'){
		strcpy(reqlinep->port, "80");
		send = cur;
	}
	if (!*send) strcpy(reqlinep->filename, "/");
	else strcpy(reqlinep->filename, send);
	return 0;
}

// seperate one header to a key : value pair
int parse_header(char *header, char *key, char *value){
	char *send = strchr(header, ':');
	if (send == NULL) return -1;
	*send = 0;
	strcpy(key, header);
	to_lowercase(key);
	*send = ':';
	strcpy(value, send+1);
	return 0;
}

int generate_requesthdrs(rio_t *rp, struct reqline_t *reqp, char *res){
	// buf stores one header
	char buf[MAXLINE], key[MAXLINE], value[MAXLINE];
	// send stores one transformed header
	char send[MAXLINE];
	int hashost = 0;

	*res = 0;
	if (rio_readlineb(rp, buf, MAXLINE)<=0) return -1;
	printf("header:%s", buf);
	while (strcmp(buf, "\r\n")){
		int err = parse_header(buf, key, value);
		if (err == -1){
			// don't change header if it's not a key:value pair
			strcpy(send, buf);
		}
		else{
			if (strcmp(key, "host") == 0){
				// don't change host
				hashost = 1;
				strcpy(send, buf);
			}
			else if (strcmp(key, "user-agent") == 0){
				strcpy(send, user_agent_hdr);
			}
			else if (strcmp(key, "connection") == 0){
				strcpy(send, "Connection: close\r\n");
			}
			else if (strcmp(key, "proxy-connection") == 0){
				strcpy(send, "Proxy-Connection: close\r\n");
			}
			else{
				strcpy(send, buf);
			}
		}
		printf("send:%s", send);
		// a new transformed header is appended to headers
		strcat(res, send);
		if (rio_readlineb(rp, buf, MAXLINE)<=0) return -1;
		printf("header:%s", buf);
	}
	// append a host if there is no host in headers
	if (!hashost){
		printf("for host:\n");
		strcpy(send, "Host: ");
		strcat(send, reqp->hostname);
		strcat(send, "\r\n");
		printf("send:%s", send);
		strcat(res, send);
	}
	strcat(res, "\r\n");
	return 0;
}

int fetch_and_pass(int fd, int myfd, Obj *obj, int *needcache){
	rio_t rio;
	rio_readinitb(&rio, myfd);
	char buf[MAXLINE];
	ssize_t nread;

	init_obj(obj);
	*needcache = 1;

	while ((nread = rio_readnb(&rio, buf, MAXLINE)) > 0){
		printf("proxy received %d bytes\n", (int)nread);
		if (needcache)
			if (cat_obj(obj, buf, nread) == -1)
				// the obj is too large to be cached
				needcache = 0;
		// pass bytes back to client
		if (rio_writen(fd, buf, nread) == -1){
			printf("cannot write back to client\n");
			return -1;
		}
	}
	if (nread == -1){
		printf("error while reading from server\n");
		return -1;
	}
	return 0;
}

/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg) 
{
	char buf[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Proxy Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The poor Proxy</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	if (rio_writen(fd, buf, strlen(buf)) == -1) return ;
	sprintf(buf, "Content-type: text/html\r\n");
	if (rio_writen(fd, buf, strlen(buf)) == -1) return ;
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	if (rio_writen(fd, buf, strlen(buf)) == -1) return ;
	if (rio_writen(fd, body, strlen(body)) == -1) return ;
}
/* $end clienterror */

