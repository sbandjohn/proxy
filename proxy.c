#include "csapp.h"
#include <assert.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void serve(int connfd);
void *thread(void *argp);
void echo(int connfd);
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg);

struct reqline_t{
	char *hostname;
	char *port;
	char *filename;
};

int parse_url(char *url, reqline_t *reqlinep){


int main(int argc, char *argv[])
{
	printf("%s", user_agent_hdr);
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;

	assert(argc == 2);
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
	Pthread_detach(pthread_self());
	serve(connfd);
	Close(connfd);
	Pthread_exit(NULL);
	return NULL;  // make gcc happy
}

void serve(int fd){
	unsigned tid = Pthread_self();
	char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
		return;
	printf("server %u got request line:%s\n", tid, buf);

	if (sscanf(buf, "%s %s %s", method, url, version) != 3){
		clienterror(fd, buf, "23333333", "Invalid request line",
				"Invalid request line!");
	}
	//line:netp:doit:parserequest
	if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
		clienterror(fd, method, "501", "Not Implemented",
				"Proxy does not implement this method");
		return;
	}                                                    //line:netp:doit:endrequesterr

	printf("server %u got url: %s\n", tid, url);
	clienterror(fd, "aha", "666", "success url is:", url);

	char hostname[MAXLINE], filename[MAXLINE], port[MAXLINE];
	struct reqline_t req;
	req.hostname = hostname;
	req.filename = filename;
	req.port = port;
	/* Parse URI from GET request */
	int err = parse_uri(uri, &req);       //line:netp:doit:staticcheck
	printf("server %u parse: hostname:%s port:%s filename%s",
			tid, req.hostname, req.port, req.filename);
}

void echo(int connfd){
	size_t n;
	char buf[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, connfd);
	while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
		printf("server received %d bytes\n", (int)n);
		Rio_writen(connfd, buf, n);
	}
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
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */