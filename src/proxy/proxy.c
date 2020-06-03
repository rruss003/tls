#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <tls.h>
#include "../rw.h"

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port portnumber -servername:serverportnumber\n", __progname);
	exit(1);
}


int bloom(char* s)
{
    return 0;
}

void action(struct tls* tls_cctx)
{
	char filename[80];
	readloop(filename, tls_cctx, 80);
	printf("Proxy recieved request for %s", filename);
	if (bloom(filename))
	{
		printf("ERROR");
	}
	else
	{
		printf("temp");
	}
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

	if (argc != 4)
		usage();
	if(strcmp(argv[1], "-port") != 0 || strncmp(argv[3], "-servername:", 12) != 0)
		usage();
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
		fprintf(stderr, "%s - value out of range\n", argv[2]);
		usage();
	}
	/* now safe to do this */
	port = p;

	/* now set up TLS */
	server_main(port, &action, "Proxy");
}