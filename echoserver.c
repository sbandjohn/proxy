#include "csapp.h"
#include <assert.h>

void echo(int connfd);

int My_open_listenfd(const char *port){
	int listenfd, optval = 1;
	struct addrinfo hints, *listp, *p;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_flags |= AI_NUMERICSERV;
	Getaddrinfo(NULL, port, &hints, &listp);

	for (p = listp; p; p = p->ai_next){
		if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;

		Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
				(const void *)&optval, sizeof(int));

		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
		Close(listenfd);
	}

	Freeaddrinfo(listp);
	if (!p)
		unix_error("Cannot open listenfd");

	if (listen(listenfd, LISTENQ) < 0){
		Close(listenfd);
		unix_error("Cannot listen");
	}
	
	return listenfd;
}


int main(int argc, char **argv){
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	char client_hostname[MAXLINE], client_port[MAXLINE];

	assert(argc == 2);

	listenfd = My_open_listenfd(argv[1]);
	while (1){
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
				client_port, MAXLINE, NI_NUMERICHOST);
		printf("connected to %s:%s\n", client_hostname, client_port);
		echo(connfd);
		Close(connfd);
		printf("disconnected\n");
	}
	exit(0);
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

