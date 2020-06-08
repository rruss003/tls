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

/* client.c  - the "classic" example of a socket client */
#include <arpa/inet.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tls.h>
#include "../lib/hash.h"



static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port portnumber filename\n", __progname);
	exit(1);
}

int main(int argc, char *argv[])
{
	struct tls_config *cfg = NULL;
	struct tls *ctx = NULL;
	struct sockaddr_in server_sa;
	char buffer[80], *ep;
	size_t maxread;
	ssize_t r, rc;
	u_short port;
	u_long p;
	int sd;
	char *filename;
	int filename_len;
	unsigned char buf[BUFSIZ];
	uint8_t *mem;
	size_t mem_len;
	ssize_t writelen;

	if (argc != 4)
		usage();
		if(strcmp(argv[1],"-port")){
			usage();
		}
        p = strtoul(argv[2], &ep, 10);
        if (*argv[2] == '\0' || *ep != '\0') {
		/* parameter wasn't a number, or was empty */
		fprintf(stderr, "%s - not a number\n", argv[2]);
		usage();
	}
        if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX)) {
		/* It's a number, but it either can't fit in an unsigned
		 * long, or is too big for an unsigned short
		 */
		fprintf(stderr, "%s - value out of range\n", argv[2]);
		usage();
	}
	/* now safe to do this */
	filename = argv[3];
	filename_len = strlen(argv[3]);
	strncpy(buf,filename,filename_len);

	//use HRW hash to determine the right proxy server address

	int chosen_port;
	chosen_port = rendezvous_hashing(buf,strlen(buf));
	printf("port chosen is %d\n",chosen_port);

	port = chosen_port;

	/*
	 * first set up "server_sa" to be the location of the server
	 */
	memset(&server_sa, 0, sizeof(server_sa));
	server_sa.sin_family = AF_INET;
	server_sa.sin_port = htons(port);
	server_sa.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (server_sa.sin_addr.s_addr == INADDR_NONE) {
		fprintf(stderr, "Invalid IP address %s\n", argv[2]);
		usage();
	}

	//initialize libtls
	if(tls_init() != 0){
		err(1,"tls_init:");
	}

	//configure libtls
	if((cfg = tls_config_new()) == NULL){
		err(1,"tls_config_new:");
	}

	//set root certificate
	if(tls_config_set_ca_file(cfg, "../certificates/root.pem") != 0){
		err(1,"tls_config_set_ca_file:");
	}

	//set client Certificates
	if(tls_config_set_cert_file(cfg, "../certificates/client.crt") != 0){
		err(1,"tls_config_set_cert_file:");
	}

	//set client certificates keys
	if(tls_config_set_key_file(cfg, "../certificates/client.key") != 0){
		err(1,"tls_config_set_key_file:");
	}

	//initialize client context
	if((ctx = tls_client()) == NULL){
		err(1,"tls_client");
	}

	//apply config to context
	if(tls_configure(ctx,cfg) != 0){
		err(1,"tls_configure: %s", tls_error(ctx));
	}
	/* ok now get a socket. we don't care where... */
	if ((sd=socket(AF_INET,SOCK_STREAM,0)) == -1)
		err(1, "socket failed");

	//connect the socket to the server described in "server_sa"
	if (connect(sd, (struct sockaddr *)&server_sa, sizeof(server_sa)) == -1)
		err(1, "connect failed");
	
	// // connect to the socket to the server
	if(tls_connect_socket(ctx,sd,"localhost") != 0){
		err(1,"tls_connect: %s", tls_error(ctx));
	}

	printf("successfully setup tls_connection to the server\n");

	//send file name to server
	if((writelen = tls_write(ctx, buf, strlen(buf))) < 0){
		err(1,"tls_write: %s", tls_error(ctx));
	}
	// //waiting for the reply and files from proxy & server
	for(;;){
		
	}
	//close the connection
	if(tls_close(ctx) != 0){
		err(1, "tls_close: %s", tls_error(ctx));
	}
	printf("connection closed\n");
	tls_free(ctx);
	tls_config_free(cfg);
	return(0);
}