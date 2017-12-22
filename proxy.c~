#include "csapp.h"
#include <assert.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

//void Perror(char *msg);

void serve(int connfd);
void *thread(void *argp);
void echo(int connfd);
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg);

struct reqline_t{
	char *scheme;
	char *hostname;
	char *port;
	char *filename;
};

int parse_uri(char *uri, struct reqline_t *reqlinep);

void to_lowercase(char *s);

int generate_requesthdrs(rio_t *rp, struct reqline_t *reqp, char *res);
int pass_back(int fd, int myfd);

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
	Pthread_detach(Pthread_self());
	serve(connfd);
	Close(connfd);
	printf("close connection\n");
	Pthread_exit(NULL);
	return NULL;  // make gcc happy
}

void serve(int fd){
	unsigned tid = Pthread_self();
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	rio_readinitb(&rio, fd);
	if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
		return;
	printf("server %u got request line: %s\n", tid, buf);

	if (sscanf(buf, "%s %s %s\r\n", method, uri, version) != 3){
		clienterror(fd, buf, "23333333", "Invalid request line",
				"Invalid request line!");
		return ;
	}

	printf("server %u got uri:(%s) version:(%s)\n", tid, uri, version);

	if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
		clienterror(fd, method, "501", "Not Implemented",
				"Proxy does not implement this method");
		return ;
	}                                                    //line:netp:doit:endrequesterr
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

	int err = parse_uri(uri, &req);       //line:netp:doit:staticcheck
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

	/* generate the headers for forwarding from the request */
	char myhdrs[MAXLINE];
	err = generate_requesthdrs(&rio, &req, myhdrs);
	printf("generated request headers:\n%sEnd\n", myhdrs);
	if (err == -1){
		clienterror(fd, "oops", "23333", "Unable to generate headers",
				"Proxy cannot generate headers from the request");
		return ;
	}

	/* try to connect to server */
	int myfd = open_clientfd(req.hostname, req.port);
	if (fd < 0){
		clienterror(fd, uri, "23333", "Unable to connect host",
				"Proxy cannot connect to host");
		return ;
	}

	char request[MAXLINE];
	sprintf(request, "GET %s HTTP/1.0\r\n", req.filename);
	strcat(request, myhdrs);
	printf("full request:\n%sEnd\n", request);

	err = rio_writen(myfd, request, strlen(request));
	if (err == -1){
		clienterror(fd, uri, "23333", "Write Error",
				"Proxy cannot write to server");
		return ;
	}
	printf("written, waiting for response\n");

	pass_back(fd, myfd);

	Close(myfd);
}

void to_lowercase(char *s){
	for (int i = 0; s[i]; ++i)
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
	char buf[MAXLINE], key[MAXLINE], value[MAXLINE];
	char send[MAXLINE];
	int hashost = 0;

	*res = 0;
	Rio_readlineb(rp, buf, MAXLINE);
	printf("header:%s", buf);
	while (strcmp(buf, "\r\n")){
		int err = parse_header(buf, key, value);
		if (err == -1){
			strcpy(send, buf);
		}
		else{
			if (strcmp(key, "host") == 0){
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
		strcat(res, send);
		Rio_readlineb(rp, buf, MAXLINE);
		printf("header:%s", buf);
	}
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

int pass_back(int fd, int myfd){
	rio_t rio;
	rio_readinitb(&rio, myfd);
	char buf[MAXLINE];
	ssize_t nread;

	while ((nread = rio_readnb(&rio, buf, MAXLINE)) > 0){
		printf("proxy received %d bytes\n", (int)nread);
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

void echo(int connfd){
	size_t n;
	char buf[MAXLINE];
	rio_t rio;

	rio_readinitb(&rio, connfd);
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

/*
void Perror(char *msg){
	sio_puts(msg);
	Pthread_exit(NULL);
}
*/
