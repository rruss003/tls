#ifndef RWH
#define RWH
#include <tls.h>
#include <err.h>
#include <stdio.h>
#include <string.h>

extern void readloop(char* buffer, struct tls* tls_ctx, ssize_t bufsize);
extern void writeloop(char* buffer, struct tls* tls_cctx, ssize_t bufsize);
#endif