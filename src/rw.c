#include "rw.h"

void readloop(char* buffer, struct tls* tls_ctx, ssize_t bufsize)
{
	ssize_t r = -1;
	ssize_t rc = 0;
	while ((r != 0) && rc < bufsize) {
		r = tls_read(tls_ctx, buffer + rc, bufsize - rc);
		if (r == TLS_WANT_POLLIN || r == TLS_WANT_POLLOUT)
			continue;
		if (r < 0)
			errx(1, "tls_read failed (%s)", tls_error(tls_ctx));
		else
			rc += r;
	}
	// printf("read %c bufsize %lu\n", buffer, rc);
}

void writeloop(char* buffer, struct tls* tls_cctx, ssize_t bufsize)
{
	// printf("write %c bufsize %lu\n", buffer, bufsize);
    ssize_t w = 0;
	ssize_t written = 0;
    while (written < bufsize) {
        w = tls_write(tls_cctx, buffer + written,
            bufsize - written);
        if (w == TLS_WANT_POLLIN || w == TLS_WANT_POLLOUT)
            continue;
        if (w < 0)
            errx(1, "tls_write failed (%s)", tls_error(tls_cctx));
        else
            written += w;
    }
}

void client_main(u_short port, const char* filename, void action(const char*, struct tls*, struct info* data)) { client_maind(port, filename, action, 0); }
void client_maind(u_short port, const char* filename, void action(const char*, struct tls*, struct info* data), struct info* data)
{
	const char* ip_address = "127.0.0.1";
	int sd, i;
	struct tls_config *tls_cfg = NULL;
	struct tls *tls_ctx = NULL;
	struct sockaddr_in server_sa;
	/*
	 * first set up "server_sa" to be the location of the server
	 */
	memset(&server_sa, 0, sizeof(server_sa));
	server_sa.sin_family = AF_INET;
	server_sa.sin_port = htons(port);
	server_sa.sin_addr.s_addr = inet_addr(ip_address);
	if (server_sa.sin_addr.s_addr == INADDR_NONE) {
		fprintf(stderr, "Invalid IP address %s\n", ip_address);
	}

	/* now set up TLS */
	if (tls_init() == -1)
		errx(1, "unable to initialize TLS");
	if ((tls_cfg = tls_config_new()) == NULL)
		errx(1, "unable to allocate TLS config");
	if (tls_config_set_ca_file(tls_cfg, "./certificates/root.pem") == -1)
		errx(1, "unable to set root CA file");
	if (tls_config_set_cert_file(tls_cfg, "./certificates/client.crt") == -1)
		errx(1, "unable to set TLS certificate file");
	if (tls_config_set_key_file(tls_cfg, "./certificates/client.key") == -1)
		errx(1, "unable to set TLS key file");

	/* ok now get a socket. we don't care where... */
	if ((sd=socket(AF_INET,SOCK_STREAM,0)) == -1)
		err(1, "socket failed");

	/* connect the socket to the server described in "server_sa" */
	if (connect(sd, (struct sockaddr *)&server_sa, sizeof(server_sa)) == -1)
		err(1, "connect failed");

	if ((tls_ctx = tls_client()) == NULL)
		errx(1, "tls client creation failed");
	if (tls_configure(tls_ctx, tls_cfg) == -1)
		errx(1, "tls configuration failed (%s)",
		    tls_error(tls_ctx));
	if (tls_connect_socket(tls_ctx, sd, "localhost") == -1) {
		errx(1, "tls connection failed (%s)",
		    tls_error(tls_ctx));
	}
	do {
		if ((i = tls_handshake(tls_ctx)) == -1)
			errx(1, "tls handshake failed (%s)",
			    tls_error(tls_ctx));
	} while (i == TLS_WANT_POLLIN || i == TLS_WANT_POLLOUT);

	action(filename, tls_ctx, data);
	close(sd);
}

static void kidhandler(int signum) {
	/* signal handler for SIGCHLD */
	waitpid(WAIT_ANY, NULL, WNOHANG);
}

void server_main(u_short port, void action(struct tls*), const char* name)
{
	struct sockaddr_in sockname, client;
	struct sigaction sa;
	int sd, i;
	socklen_t clientlen;
	pid_t pid;
	struct tls_config *tls_cfg = NULL;
	struct tls *tls_ctx = NULL;
	struct tls *tls_cctx = NULL;
	/* now set up TLS */

	if ((tls_cfg = tls_config_new()) == NULL)
		errx(1, "unable to allocate TLS config");
	if (tls_config_set_ca_file(tls_cfg, "./certificates/root.pem") == -1)
		errx(1, "unable to set root CA file");
	if (tls_config_set_cert_file(tls_cfg, "./certificates/server.crt") == -1)
		errx(1, "unable to set TLS certificate file");
	if (tls_config_set_key_file(tls_cfg, "./certificates/server.key") == -1)
		errx(1, "unable to set TLS key file");
	if ((tls_ctx = tls_server()) == NULL)
		errx(1, "tls server creation failed");

	tls_config_verify_client_optional(tls_cfg);
	
	if (tls_configure(tls_ctx, tls_cfg) == -1)
		errx(1, "tls configuration failed (%s)", tls_error(tls_ctx));

	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);
	sd=socket(AF_INET,SOCK_STREAM,0);
	if ( sd == -1)
		err(1, "socket failed");

	if (bind(sd, (struct sockaddr *) &sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");

	if (listen(sd,3) == -1)
		err(1, "listen failed");

	/*
	 * we're now bound, and listening for connections on "sd" -
	 * each call to "accept" will return us a descriptor talking to
	 * a connected client
	 */


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

	printf("%s: listening for connections on port %u\n", name, port);
	/*
	 * finally - the main loop.  accept connections and deal with 'em
	 */
	 server_loop(tls_ctx, tls_cctx, sd, action);
}

void server_loop(struct tls *tls_ctx, struct tls *tls_cctx, int sd, void action(struct tls*))
{
	struct sockaddr_in client;
	socklen_t clientlen;
	pid_t pid;
	int i;
	for(;;) {
		clientlen = sizeof(&client);
		int clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		/*
		 * We fork child to deal with each connection, this way more
		 * than one client can connect to us and get served at any one
		 * time.
		 */
		pid = fork();
		if (pid == -1)
		     err(1, "fork failed");
		if(pid == 0) {
			i = 0;
			if (tls_accept_socket(tls_ctx, &tls_cctx, clientsd) == -1)
				errx(1, "tls accept failed (%s)", tls_error(tls_ctx));
			else {
				do {
					if ((i = tls_handshake(tls_cctx)) == -1)
						errx(1, "tls handshake failed (%s)",
						    tls_error(tls_cctx));
				} while(i == TLS_WANT_POLLIN || i == TLS_WANT_POLLOUT);
			}

			action(tls_cctx);
			
			i = 0;
			do {
				i = tls_close(tls_cctx);
			} while(i == TLS_WANT_POLLIN || i == TLS_WANT_POLLOUT);

			close(clientsd);
			exit(0);
		}
		close(clientsd);
	}
}

u_short get_port(char* portstr, void usage())
{
	char *ep;
	u_long p;
	/*
	 * first, figure out what port we will listen on - it should
	 * be our first parameter.
	 */
	errno = 0;
	p = strtoul(portstr, &ep, 10);
	if (*portstr == '\0' || *ep != '\0') {
		/* parameter wasn't a number, or was empty */
		fprintf(stderr, "%s - not a number\n", portstr);
		usage();
	}
    if ((errno == ERANGE && p == ULONG_MAX) || (p > USHRT_MAX)) {
		/* It's a number, but it either can't fit in an unsigned
		 * long, or is too big for an unsigned short
		 */
		fprintf(stderr, "%s - value out of range\n", portstr);
		usage();
	}
	return (u_short) p;
}