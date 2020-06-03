/*
 * Copyright (c) 2015 Bob Beck <beck@obtuse.com>
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
 * compile with gcc -o server server.c -ltls -lssl -lcrypto
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
#include "../rw.h"

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s portnumber\n", __progname);
	exit(1);
}


void action(struct tls* tls_cctx)
{
	#define wl(a) writeloop((char*)&a, tls_cctx, sizeof(a))
	printf("action called\n");
	char filename[80];
	readloop(filename, tls_cctx, 80);

	FILE *f = fopen(filename, "rb");
	char status = -1;
	if(f==NULL)
	{
		wl(status);
		errx(1, "Invalid filename");
	}
	fseek(f, 0, SEEK_END);
	size_t filesize = ftell(f);
	rewind(f);
	char* buffer = malloc(filesize);
	
	fread(buffer, sizeof(char), filesize, f);
	if ( ferror( f ) != 0 )
	{
		wl(status);
		errx(1, "Error reading file");
	}

	fclose(f);
	printf("server fs: %ld\n", filesize);
	// printf("BUFFER: %s\n", buffer);
	status = 0;
	wl(status);
	wl(filesize);
	writeloop(buffer, tls_cctx, filesize);
	
	free(buffer);
}

int main(int argc,  char *argv[])
{
	char *ep;
	u_short port;
	u_long p;

	/*
	 * first, figure out what port we will listen on - it should
	 * be our first parameter.
	 */
	if (argc != 2)
		usage();
		errno = 0;
        p = strtoul(argv[1], &ep, 10);
        if (*argv[1] == '\0' || *ep != '\0') {
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
	server_main(port, &action, "Server");
}