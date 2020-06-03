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
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <tls.h>
#include "../rw.h"
#include "../hash.h"

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port proxyportnumber filename\n", __progname);
	exit(1);
}

void action(const char* fname, struct tls* tls_ctx)
{
	#define rl(a) readloop((char*)&a, tls_ctx, sizeof(a))
	char filename[80];
	strncpy(filename, fname, 80);
	writeloop(filename, tls_ctx, 80);
	
	char status = -1;
	rl(status);
	if(status != 0)
		errx(1, "client: Invalid filename");

	size_t filesize = 0;
	rl(filesize);
	printf("%ld\n", filesize);

	char *buffer = malloc(filesize);
	readloop(buffer, tls_ctx, filesize);
	buffer[filesize] = '\0';
	printf("%s\n", buffer);
	free(buffer);
}

int main(int argc, char *argv[])
{
	char *ep;
	u_long p;
	int sd, i;

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
	client_main(hash(argv[3], argv[2]), argv[3], &action);
	return(0);
}