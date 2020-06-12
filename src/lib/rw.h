#ifndef __RW_H__
#define __RW_H__

#include <tls.h>
#include <err.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>

int sendfile(struct tls* ctx, char *path);
int readfile(struct tls* ctx, char *path);
void readloop(char* buffer, struct tls* tls_ctx, ssize_t bufsize);
void writeloop(char* buffer, struct tls* tls_cctx, ssize_t bufsize);

#endif // DEBUG