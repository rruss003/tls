/*
 * Copyright (c) 2008 Bob Beck <beck@obtuse.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* server.c  - the "classic" example of a socket server */

/*
 * compile with gcc -o server server.c
 * or if you are on a crappy version of linux without strlcpy
 * thanks to the bozos who do glibc, do
 * gcc -c strlcpy.c
 * gcc -o server server.c strlcpy.o
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tls.h>
#include "../lib/rw.h"

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port portnumber\n", __progname);
	exit(1);
}

static void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}


int main(int argc,  char *argv[])
{
	struct sockaddr_in sockname, client;
	struct tls_config *cfg = NULL;
	struct tls *ctx = NULL, *cctx = NULL;
	char buffer[80], *ep;
	struct sigaction sa;
	int sd;
	socklen_t clientlen;
	u_short port;
	pid_t pid;
	u_long p;
	uint8_t *mem;
	size_t mem_len;
	unsigned char buf[BUFSIZ];

	/*
	 * first, figure out what port we will listen on - it should
	 * be our first parameter.
	 */

	if (argc != 3)
		usage();
		if(strcmp(argv[1],"-port")){
			usage();
		}
		errno = 0;
        p = strtoul(argv[2], &ep, 10);
        if (*argv[2] == '\0' || *ep != '\0') {
		/* parameter wasn't a number, or was empty */
		fprintf(stderr, "%s - not a number\n", argv[1]);
		usage();
		}
        if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX)) {
		/* It's a number, but it either can't fit in an unsigned
		 * long, or is too big for an unsigned short
		 */
		fprintf(stderr, "%s - value out of range\n", argv[1]);
		usage();
		}
	/* now safe to do this */
	port = p;
	// initialize libtls
	if(tls_init()!=0)
		err(1,"tls_init:");
	// configure libTLS
	if((cfg = tls_config_new()) == NULL)
		err(1,"tls_config_new:");
	//set root certificate
	if(tls_config_set_ca_file(cfg,"../certificates/root.pem") != 0)
		err(1,"tls_config_set_ca_file failed:");
	//set server certificates
	if(tls_config_set_cert_file(cfg,"../certificates/server.crt") != 0)
		err(1,"tls_config_set_cert_file failed:");
	//set server private keys
	if(tls_config_set_key_file(cfg,"../certificates/server.key") != 0)
		err(1,"tls_configure_set_key_file failed:");
	//initialize server context
	if((ctx = tls_server()) == NULL)
		err(1, "tls_server:");
	//apply config to context
	if(tls_configure(ctx,cfg) != 0)
		err(1, "tls_configure: %s", tls_error(ctx));

	//setup the socket
	printf("setting up socket!\n");
	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd=socket(AF_INET,SOCK_STREAM,0);
	if (sd == -1)
		err(1, "socket failed");
	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");
	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */
	if (listen(sd,3) == -1)
		err(1, "listen failed");
	/*
	 * first, let's make sure we can have children without leaving
	 * zombies around when they die - we can do this by catching
	 * SIGCHLD.
	 */
	sa.sa_handler = kidhandler;
        sigemptyset(&sa.sa_mask);
	/*
	 * we want to allow system calls like accept to be restarted if they
	 * get interrupted by a SIGCHLD
	 */
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1)
                err(1, "sigaction failed");

	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
	printf("Server: setup and listening for connections on port %u\n", port);
	for(;;) {
		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */
		int clientsd;
		clientlen = sizeof(&client);
		printf("Server: ready to accept\n");
		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		printf("Server: successfully connected\n");
		if(tls_accept_socket(ctx,&cctx,clientsd) != 0)
			err(1, "tls_accept_socket failed: %s",tls_error(ctx));
		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");
		if(pid == 0){
			//waiting for message from proxy
			ssize_t readlen;
			if((readlen = tls_read(cctx, buf, sizeof(buf))) < 0){
				err(1, "tls_read:%s", tls_error(ctx));
			}
			printf("Server: Requested file name: [%s] \n", buf);
			char file_path[256];
			strcpy(file_path,"../src/server/");
			strncat(file_path, buf, strlen(buf));
			int ret;
			ret = sendfile(cctx, file_path);
			if(ret < 0){
			printf("Server: please terminate the client\n");
			}
			close(clientsd);
			exit(0);
		}
		close(clientsd);
	}
}
