#ifndef RWH
#define RWH
#include <tls.h>
#include <string.h>

extern void readloop(char* buffer, struct tls* tls_cctx);
extern void writeloop(char* buffer, struct tls* tls_cctx);
#endif