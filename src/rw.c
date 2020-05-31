#include "rw.h"
/*
* finally, we are connected. find out what magnificent wisdom
* our server is going to send to us - since we really don't know
* how much data the server could send to us, we have decided
* we'll stop reading when either our buffer is full, or when
* we get an end of file condition from the read when we read
* 0 bytes - which means that we pretty much assume the server
* is going to send us an entire message, then close the connection
* to us, so that we see an end-of-file condition on the read.
*
* we also make sure we handle EINTR in case we got interrupted
* by a signal.
*/
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
}

/*
* write the message to the client, being sure to
* handle a short write, or being interrupted by
* a signal before we could write anything.
*/
void writeloop(char* buffer, struct tls* tls_cctx, ssize_t bufsize)
{
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