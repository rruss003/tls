#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <tls.h>
#include "rw.h"



int readfile(struct tls* ctx, char *path){
	FILE *fp =fopen(path, "w");
	int valid = 0;
	char buf[4096];
	// char linesize[32];
	for(;;){
		// tls_read(ctx, linesize, sizeof(linesize));
		// printf("%s\n", linesize);
		ssize_t r;
		// r = tls_read(ctx, buf, atoi(linesize));
		r = tls_read(ctx, buf, sizeof(buf));
		if(r == TLS_WANT_POLLIN || r == TLS_WANT_POLLOUT)
			continue;
		if(r < 0){
			err(1, "tls_read: %s", tls_error(ctx));
			return -1;
		}
		if(r == 0){
			if(valid == 1){
				break;
			}
			continue;
		}
		else{
			fprintf(fp,"%s", buf);
			valid = 1;
			memset(buf, 0, sizeof(buf));
		}
	}
	fclose(fp);
	return 0;
}

int sendfile(struct tls* ctx, char *path){
	char buf[4096];
	// char linesize[32];
	FILE *fp =fopen(path, "r");
	if(fp == NULL){
		printf("Can't find the requested file\n");
		return -1;
	}
	ssize_t r;
	while(fgets(buf, sizeof(buf), fp) != NULL){
		// snprintf(linesize, sizeof(linesize), "%u", strlen(buf));
		if(strlen(buf) < sizeof(buf)){
			// tls_write(ctx, linesize, sizeof(linesize));
			printf("%s", buf);
			// r = tls_write(ctx, buf, atoi(linesize));
			r = tls_write(ctx,buf, strlen(buf));
			if(r== TLS_WANT_POLLIN || r== TLS_WANT_POLLOUT)
				continue;
		}
	}
	fclose(fp);
	printf("\nFinish transmission\n");
	return 0;
}

