#ifndef RWH
#define RWH
#include <tls.h>
#include <err.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <errno.h>
#include <limits.h>

struct info
{
    int status;
    size_t filesize;
    char* data;
};
extern void readloop(char* buffer, struct tls* tls_ctx, ssize_t bufsize);
extern void writeloop(char* buffer, struct tls* tls_cctx, ssize_t bufsize);
extern void client_main(u_short port, const char* filename, void action(const char*, struct tls*, struct info*));
extern void client_maind(u_short port, const char* filename, void action(const char*, struct tls*, struct info*), struct info* data);
extern void server_main(u_short port, void action(struct tls*), const char* name);
extern void server_loop(struct tls *tls_ctx, struct tls *tls_cctx, int sd, void action(struct tls*));
u_short get_port(char* portstr, void usage());
#endif