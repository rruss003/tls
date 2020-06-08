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
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <tls.h>
#include "../rw.h"
#include "../hash.h"

void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port proxyportnumber filename\n", __progname);
	exit(1);
}

void action(const char* fname, struct tls* tls_ctx, struct info* data)
{
	#define rl(a) readloop((char*)&(a), tls_ctx, sizeof(a))
	char filename[80];
	strncpy(filename, fname, 80);
	writeloop(filename, tls_ctx, 80);
	
	/*
	char status = -1;
	// rl(status);
	fprintf(stderr, "client status = %d\n", status);
	sprintf(filename, "%d", status);
	#define BIG = 100
	size_t filesize = status;
	// if(status != 69)
	// 	errx(1, filename);

	// size_t filesize = 0;
	// rl(filesize);
	// printf("Client: %s filesize: 0x%7x\n", fname, filesize);

	// char *buffer = malloc(filesize);
	char buffer[BIG];
	readloop(buffer, tls_ctx, BIG);
	buffer[filesize] = '\0';
	printf("Client: %s contents:\n%s\n", fname, buffer);
	free(buffer);
	*/
	int status = -1;
	size_t filesize;
	// printf("BEFORE READS\n");
	rl(status);
	if (status != 0)
		errx(1, "Proxy returned error - Invalid filename");
	rl(filesize);
	printf("Client: %s filesize: %lu\n", fname, filesize);
	char* buffer = malloc(filesize);
	readloop(buffer, tls_ctx, filesize);
	printf("Client: %s contents:\n%s\n", fname, buffer);
	free(buffer);
}

int main(int argc, char *argv[])
{
	if (argc != 4)
		usage();
	if(strcmp(argv[1], "-port") != 0)
		usage();

	u_short port = rendezvous_hash(argv[3]);
	printf("Client: requesting %s on port %u\n", argv[3], port);
	client_main(port, argv[3], &action);
	return(0);
}