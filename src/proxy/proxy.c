#include <err.h>
#include <stdio.h>
#include <string.h>
#include <tls.h>
#include <stdbool.h>
#include "../rw.h"
#include "../bloomfilter.h"

#define MAX_CONNECTIONS 5
u_short server_port;
char* data[MAX_CONNECTIONS];
uint32_t hashtable[ARRAYSIZE];
char hpath[80];


void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port portnumber -servername:serverportnumber\n", __progname);
	exit(1);
}

void client_action(const char* fname, struct tls* tls_cctx, struct info* data)
{
	#define rl(a) readloop((char*)&(a), tls_cctx, sizeof(a))
	char filename[80];
	strncpy(filename, fname, 80);
	writeloop(filename, tls_cctx, 80);
	
	char status = -1;
	rl(status);
	data->status = status;
	if(status != 0)
		errx(1, "Proxy: Server returned error - Invalid filename");

	size_t filesize = 0;
	rl(filesize);
	data->filesize = filesize;

	char *buffer = malloc(filesize);
	readloop(buffer, tls_cctx, filesize);
	data->data = buffer;
}

void server_action(struct tls* tls_cctx)
{
	#define wl(a) writeloop((char*)&(a), tls_cctx, sizeof(a))
	char filename[80];
	readloop(filename, tls_cctx, 80);
	printf("Proxy: recieved request for %s\n", filename);
	bloomfilter_init(hashtable);
	readarrayfromfile(hashtable, "./hash.txt");
	
	if (bloomfilter_query(hashtable, filename, strlen(filename)) == 0)
	{
		FILE *f = NULL;
		if((f=fopen(filename, "rb")) == NULL)
			printf("Proxy: bloom filter returned false positive!\n");
		else
		{
			int status = 0;
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
			printf("Proxy: returning filesize: %lu\n", filesize);
			status = 0;
			wl(status);
			printf("Proxy: returning file contents:\n%s\n", buffer);
			wl(filesize);
			writeloop(buffer, tls_cctx, filesize);
			free(buffer);
			return;
		}
	}
	struct info data;
	int status = -1;
	printf("Proxy: requesting %s on port %u\n", filename, server_port);
	client_maind(server_port, filename, &client_action, &data);
	if (status != 0)
	{
		bloomfilter_insert(hashtable, filename, strlen(filename));
		writearraytofile(hashtable, "./hash.txt");
	}
	wl(data.status);
	printf("Proxy: returning filesize: %lu\n", data.filesize);
	wl(data.filesize);
	printf("Proxy: returning file contents:\n%s\n", data.data);
	writeloop(data.data, tls_cctx, 6);
}

int main(int argc,  char *argv[])
{
	if (argc != 4)
		usage();
	if(strcmp(argv[1], "-port") != 0 || strncmp(argv[3], "-servername:", 12) != 0)
		usage();

	server_port = get_port(argv[3]+12, &usage);

	server_main(get_port(argv[2], &usage), &server_action, "Proxy");
}