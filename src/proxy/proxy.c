
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
#include <stdint.h>
#include <tls.h>
#include "../lib/hash.h"
#include "../lib/bloomfilter.h"
#include "../lib/rw.h"

#define CACHEPATH "../src/proxy/"

struct proxy proxylist;
uint32_t hashtable[ARRAYSIZE];
char bfpath[256];


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
	remove(bfpath);
	printf("\nProxy: ready to quit\n");
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

	if (argc != 4)
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
		char server[256];
		char servername[256];
		char *result = NULL;
		char serverport[64];
		strncpy(server,argv[3]+1,strlen(argv[3])-1);
		if(argv[3][0]!='-'){
			usage();
		}
		result = strtok(server,":");
		strncpy(servername,result,strlen(result));
		result = strtok(NULL,":");
		strncpy(serverport,result,strlen(result));


	/* now safe to do this */
	port = p;
	// intall INT signal handler
	signal(SIGINT,int_handler);
	// initialize libtls
	if(tls_init()!=0)
		err(1,"tls_init:");
	// configure libTLS
	if((cfg = tls_config_new()) == NULL)
		err(1,"tls_config_new:");
	//set root certificate
	if(tls_config_set_ca_file(cfg,"../certificates/root.pem") != 0)
		err(1,"tls_config_set_ca_file failed:");
	//set proxy certificates
	if(tls_config_set_cert_file(cfg,"../certificates/server.crt") != 0)
		err(1,"tls_config_set_cert_file failed:");
	//set proxy private keys
	if(tls_config_set_key_file(cfg,"../certificates/server.key") != 0)
		err(1,"tls_configure_set_key_file failed:");
	//initialize proxy context
	if((proxy_ctx = tls_server()) == NULL)
		err(1, "tls_server:");
	//apply config to context
	if(tls_configure(proxy_ctx,cfg) != 0)
		err(1, "tls_configure: %s", tls_error(proxy_ctx));
	//setup the socket
	printf("setting up socket!\n");
	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd=socket(AF_INET,SOCK_STREAM,0);
	if (sd == -1)
		err(1, "socket failed");
	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");
	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */
	if (listen(sd,3) == -1)
		err(1, "listen failed");
	sa.sa_handler = kidhandler;
        sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
            err(1, "sigaction failed");

	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
	printf("Proxy: setup and listening for connections on port %u\n", port);

	//add an entry to the list
	strcpy(proxylist.addr,"127.0.0.1");
	proxylist.port = port;
	strcpy(proxylist.name,"proxy");
	writelisttofile();
	//covert lsitening port to string
    char cport[64];
    snprintf(cport, sizeof(cport),"%d",port);
	//get the tmp file (store hash table) path
    strcpy(bfpath, CACHEPATH);
    strcat(bfpath, cport);
	// initialize bloom_filter
	bloomfilter_init(hashtable);
	writearraytofile(hashtable,bfpath);

	for(;;){
		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */
		int clientsd;
		clientlen = sizeof(&client);
		printf("Proxy: ready to accept socket!\n");
		clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		printf("Proxy: successfully connected with a client\n");
		if(tls_accept_socket(proxy_ctx,&cctx,clientsd) != 0)
			err(1, "tls_accept_socket failed: %s",tls_error(proxy_ctx));
		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");

		if(pid == 0){
			// read current bloom filter hash map
			readarrayfromfile(hashtable,bfpath);
			//waiting for message from client
			ssize_t readlen;
			if((readlen = tls_read(cctx, buf, sizeof(buf))) < 0)
				err(1, "tls_read:%s", tls_error(proxy_ctx));
			printf("Prxoy: client requested file name: [%s] \n", buf);

			char request[256];
			strncpy(request, buf, strlen(buf));
			//use bloom filter here
			int queryresult = bloomfilter_query(hashtable,request,strlen(request));
			char tmp_path[256];
			strcpy(tmp_path,"../src/proxy/");
			strncat(tmp_path, request, strlen(request));
			if(queryresult == 0){
				char fpath[256];
				strcpy(fpath,"../");
				strcat(fpath,request);
				FILE *fp = NULL;
				if((fp=fopen(fpath,"r")) == NULL){
					printf("Proxy: bloom filter return false positive!\n");
					queryresult = -1;
				}
			}
			if(queryresult == -1){
				printf("Proxy: request file: [%s] from server\n", request);
				//request the file from the server
				// fork a new child to connect to the server
				int s_pid = fork();
				if(s_pid == 0){
					printf("Proxy: server name: %s, server port: %s\n", servername,serverport);
					if((proxy_ctx = tls_client()) == NULL)
						err(1,"tls_client");
					if(tls_configure(proxy_ctx,cfg) != 0)
						err(1,"tls_configure: %s", tls_error(proxy_ctx));
					if(tls_connect(proxy_ctx,servername,serverport)!=0)
						err(1, "tls_connect: %s", tls_error(proxy_ctx));
					printf("Proxy: connected to the server!\n");
					if((tls_write(proxy_ctx, request, strlen(request))) < 0)
						err(1,"tls_write: %s", tls_error(proxy_ctx));
					readfile(proxy_ctx, tmp_path);
					if(tls_close(proxy_ctx)!=0)
					bloomfilter_insert(hashtable, request, strlen(request));
					writearraytofile(hashtable, bfpath);
					printf("\n");
					printf("Proxy: received file [%s] from server and ready to send to client\n", request);
					exit(0);
				}
			}
			//send the file to the client
			waitpid(-1, 0, 0);
			sendfile(cctx, tmp_path);
			printf("Proxy: send file [%s] to client\n", request);
			close(clientsd);
			exit(0);
		}
		close(clientsd);
	}
}