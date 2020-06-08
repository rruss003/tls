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
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <tls.h>
#include "../rw.h"

void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s portnumber\n", __progname);
	exit(1);
}


void action(struct tls* tls_cctx)
{
	#define wl(a) writeloop((char*)&(a), tls_cctx, sizeof(a))
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
	size_t filesize = ftell(f) + 800;
	rewind(f);
	char* buffer = malloc(filesize);
	
	fread(buffer, sizeof(char), filesize, f);
	if ( ferror( f ) != 0 )
	{
		wl(status);
		errx(1, "Error reading file");
	}

	fclose(f);
	printf("Server: returning filesize: %lu\n", filesize);
	// printf("BUFFER: %s\n", buffer);
	status = 0;
	wl(status);
	printf("Server: returning file contents:\n%s\n", buffer);
	wl(filesize);
	writeloop(buffer, tls_cctx, filesize);
	
	free(buffer);
}

int main(int argc,  char *argv[])
{
	if (argc != 2)
		usage();

	server_main(get_port(argv[1], &usage), &action, "Server");
}