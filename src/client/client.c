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
#include "../rw.h"


static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port proxyportnumber filename\n", __progname);
	exit(1);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in server_sa;
	char *ep;
	u_short port;
	u_long p;
	int sd, i;
	struct tls_config *tls_cfg = NULL;
	struct tls *tls_ctx = NULL;
	char* IPADDR = "127.0.0.1";

	if (argc != 4)
		usage();

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
	port = p;

	/*
	 * first set up "server_sa" to be the location of the server
	 */
	memset(&server_sa, 0, sizeof(server_sa));
	server_sa.sin_family = AF_INET;
	server_sa.sin_port = htons(port);
	server_sa.sin_addr.s_addr = inet_addr(IPADDR);
	if (server_sa.sin_addr.s_addr == INADDR_NONE) {
		fprintf(stderr, "Invalid IP address %s\n", IPADDR);
		usage();
	}

	/* now set up TLS */
	if (tls_init() == -1)
		errx(1, "unable to initialize TLS");
	if ((tls_cfg = tls_config_new()) == NULL)
		errx(1, "unable to allocate TLS config");
	if (tls_config_set_ca_file(tls_cfg, "./certificates/root.pem") == -1)
		errx(1, "unable to set root CA file");
	if (tls_config_set_cert_file(tls_cfg, "./certificates/client.crt") == -1)
		errx(1, "unable to set TLS certificate file");
	if (tls_config_set_key_file(tls_cfg, "./certificates/client.key") == -1)
		errx(1, "unable to set TLS key file");

	/* ok now get a socket. we don't care where... */
	if ((sd=socket(AF_INET,SOCK_STREAM,0)) == -1)
		err(1, "socket failed");

	/* connect the socket to the server described in "server_sa" */
	if (connect(sd, (struct sockaddr *)&server_sa, sizeof(server_sa)) == -1)
		err(1, "connect failed");

	if ((tls_ctx = tls_client()) == NULL)
		errx(1, "tls client creation failed");
	if (tls_configure(tls_ctx, tls_cfg) == -1)
		errx(1, "tls configuration failed (%s)",
		    tls_error(tls_ctx));
	if (tls_connect_socket(tls_ctx, sd, "localhost") == -1) {
		errx(1, "tls connection failed (%s)",
		    tls_error(tls_ctx));
	}
	do {
		if ((i = tls_handshake(tls_ctx)) == -1)
			errx(1, "tls handshake failed (%s)",
			    tls_error(tls_ctx));
	} while (i == TLS_WANT_POLLIN || i == TLS_WANT_POLLOUT);
	char filename[80];
	strncpy(filename, argv[3], 80);
	writeloop(filename, tls_ctx, 80);

	ssize_t filesize = 0;
	readloop((char*)&filesize, tls_ctx, sizeof(filesize));
	printf("%ld\n", filesize);

	char *buffer = malloc(filesize);
	readloop(buffer, tls_ctx, filesize);

	FILE* file = fopen("./test", "wb");
	if (file != NULL) {
		fwrite(buffer, filesize, 1, file);
	}

	fclose(file);
	free(buffer);
	close(sd);
	return(0);
}