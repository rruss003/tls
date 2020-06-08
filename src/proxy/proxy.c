
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tls.h>
#include "../hash.h"

struct proxy proxylist;

// add a line to the proxylist file, since using a file is the simplest way to inform other proxies and client about current proxies in running 
int writelisttofile(){
	FILE *fp = NULL;
	fp=fopen("../src/proxylist.txt", "a+"); 
	if(fprintf(fp,"%d\t%s\t%s\n",proxylist.port,proxylist.addr,proxylist.name) == 0){
		fclose(fp);
		return 0;
	}else{
		fclose(fp);
		return -1;
	}
}

static void usage()
{
	extern char * __progname;
	fprintf(stderr, "usage: %s -port portnumber\n", __progname);
	exit(1);
}

static void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}

// when we use Ctrl+C to terminate the proxy server
void int_handler(int sig){
	//delete the entry in proxylist.
	char line[256];
	char tmp[256];
	FILE *fp=NULL;
	FILE *fp_tmp=NULL;
	fp=fopen("../src/proxylist.txt", "r");
	fp_tmp=fopen("../src/temp.txt", "w+");
	// check every line in the file, if the port(first string in every line) is not the port this proxy listening, copy it to a temporary file, and overwrite the original file at the end
	while(fgets(line, sizeof(line), fp) != NULL){
		strcpy(tmp, line);
		char *result=strtok(line,"\t");
		if(atoi(result) != proxylist.port){
			fprintf(fp_tmp,"%s",tmp);
		}
	}
	fclose(fp_tmp);
	fclose(fp);
	// remove the original file
	remove("../src/proxylist.txt");
	// overwirte by rename temporary file
	rename("../src/temp.txt", "../src/proxylist.txt");
	printf("\nready to quit\n");
	exit(0);
}

int main(int argc,  char *argv[])
{
	struct sockaddr_in sockname, client;
	struct tls_config *cfg = NULL;
	struct tls *proxy_ctx = NULL, *cctx = NULL;
	char buffer[80], *ep;
	struct sigaction sa;
	int sd;
	socklen_t clientlen;
	u_short port;
	pid_t pid;
	u_long p;
	uint8_t *mem;
	size_t mem_len;
	unsigned char buf[BUFSIZ];

	/*
	 * first, figure out what port we will listen on - it should
	 * be our first parameter.
	 */

	if (argc != 3)
		usage();
		if(strcmp(argv[1],"-port")){
			usage();
		}
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
		fprintf(stderr, "%s - value out of range\n", argv[1]);
		usage();
	}
	/* now safe to do this */
	port = p;
	// intall INT signal handler
	signal(SIGINT,int_handler);
	// initialize libtls
	if(tls_init()!=0){
		err(1,"tls_init:");
	}
	// configure libTLS
	if((cfg = tls_config_new()) == NULL){
		err(1,"tls_config_new:");
	}
	//set root certificate
	if(tls_config_set_ca_file(cfg,"../certificates/root.pem") != 0){
		err(1,"tls_config_set_ca_file failed:");
	}
	//set proxy certificates
	if(tls_config_set_cert_file(cfg,"../certificates/server.crt") != 0){
		err(1,"tls_config_set_cert_file failed:");
	}
	//set proxy private keys
	if(tls_config_set_key_file(cfg,"../certificates/server.key") != 0){
		err(1,"tls_configure_set_key_file failed:");
	}
	//initialize proxy context
	if((proxy_ctx = tls_server()) == NULL){
		err(1, "tls_server:");
	}
	//apply config to context
	if(tls_configure(proxy_ctx,cfg) != 0){
		err(1, "tls_configure: %s", tls_error(proxy_ctx));
	}
	//setup the socket
	printf("setting up socket!\n");
	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd=socket(AF_INET,SOCK_STREAM,0);
	if (sd == -1){
		err(1, "socket failed");
	}
	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");
	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */
	if (listen(sd,3) == -1)
		err(1, "listen failed");
	/*
	 * first, let's make sure we can have children without leaving
	 * zombies around when they die - we can do this by catching
	 * SIGCHLD.
	 */
	sa.sa_handler = kidhandler;
        sigemptyset(&sa.sa_mask);
	/*
	 * we want to allow system calls like accept to be restarted if they
	 * get interrupted by a SIGCHLD
	 */
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1)
                err(1, "sigaction failed");

	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
	printf("proxy up and listening for connections on port %u\n", port);
	strcpy(proxylist.addr,"127.0.0.1");
	proxylist.port = port;
	strcpy(proxylist.name,"proxy");
	writelisttofile();

	for(;;) {
		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */
		int clientsd;
		clientlen = sizeof(&client);
		printf("ready to accept\n");
		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		printf("successfully connected\n");
		if(tls_accept_socket(proxy_ctx,&cctx,clientsd) != 0){
			err(1, "tls_accept_socket failed: %s",tls_error(proxy_ctx));
		}
		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");

		if(pid == 0) {

			//waiting for message from client
			ssize_t readlen;
			if((readlen = tls_read(cctx, buf, sizeof(buf))) < 0){
				err(1, "tls_read:%s", tls_error(proxy_ctx));
			}
			printf("requested file name: [%s] \n", buf);

			//bloom filter use here



			ssize_t written, w;
			/*
			 * write the message to the client, being sure to
			 * handle a short write, or being interrupted by
			 * a signal before we could write anything.
			 */
			
			close(clientsd);
			exit(0);
		}
		close(clientsd);
	}
}
