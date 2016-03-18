/* xbs_socket.c
 * Copyright by beyondy, 2004-2010
 * All rights reserved.
 *
**/
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <beyondy/xbs_io.h>
#include <beyondy/xbs_naddr.h>
#include <beyondy/xbs_socket.h>

namespace beyondy {

/* base wrapper */
int XbsSocket(int domain, int type, int protocol)
{
	return socket(domain, type, protocol);
}

int XbsBind(int fd, const char *address)
{
	struct sockaddr_storage addr_buf;
	socklen_t addr_len = sizeof(addr_buf);

	if (str2sockaddr(address, NULL, (struct sockaddr *)&addr_buf, &addr_len, 1) < 1) return -1;
	return bind(fd, (struct sockaddr *)&addr_buf, addr_len);
}

int XbsListen(int fd, int backlog)
{
	return listen(fd, backlog);
}

int XbsAccept(int fd, long timedout)
{
	if (XbsWaitReadable(fd, timedout) < 0) return -1;
	return accept(fd, NULL, NULL);
}

/* if caller want to connect in limited timeout, it should set fd nonblocking
 * otherwise, this function will do blocking connect
**/
int XbsConnect(int fd, const struct sockaddr *target, socklen_t alen, long timedout)
{
	int retval = connect(fd, target, alen);
	if (retval == -1) {
		if (timedout < 1)
			return 0;	// with errno

		if (errno == EINPROGRESS) {
			if (XbsWaitWritable(fd, timedout) < 0) {
				return -1;
			}

			if ((retval = XbsGetError(fd)) != 0) {
				errno = retval;
				return -1;
			}

			errno = 0;	// clear errno
			return 0;
		}
		else {
			return -1;
		}
	}
	
	errno = 0;	// clear errno
	return 0;	// good?!
}

int XbsConnect(int fd, const char *target, long timedout)
{
	struct sockaddr_storage addr_buf;
	socklen_t addr_len = sizeof(addr_buf);

	if (str2sockaddr(target, NULL, (struct sockaddr *)&addr_buf, &addr_len, 1) < 1) return -1;
	return XbsConnect(fd, (struct sockaddr *)&addr_buf, addr_len, timedout);
}

// TODO: wrapper more socket api?

/* get peer & local name */
int XbsGetPeername(int fd, char *nbuf, size_t size)
{
	struct sockaddr_storage addr_buf;
	int type = 0;
	socklen_t addr_len = sizeof(addr_buf);
	socklen_t tlen = sizeof(type);

	if (getpeername(fd, (struct sockaddr *)&addr_buf, &addr_len) < 0) return -1;
	if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &tlen) < 0) return -1;

	return sockaddr2str((struct sockaddr *)&addr_buf, type, nbuf, size);
}

int XbsGetLocalname(int fd, char *nbuf, size_t size)
{
	struct sockaddr_storage addr_buf;
	int type = 0;
	socklen_t addr_len = sizeof(addr_buf);
	socklen_t tlen = sizeof(type);

	if (getsockname(fd, (struct sockaddr *)&addr_buf, &addr_len) < 0) return -1;
	if (getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &tlen) < 0) return -1;

	return sockaddr2str((struct sockaddr *)&addr_buf, type, nbuf, size);
}

/* flags & other socket ops */
int XbsSetFlags(int fd, int on, int off)
{
	int flags = fcntl(fd, F_GETFL);
	flags |= on; flags &= ~off;
	return fcntl(fd, F_SETFL, flags);
}

int XbsSetReuse (int fd, int reuse)
{
	int address = (reuse & XBS_REUSE_ADDR) ? 1 : 0;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &address, sizeof(address)) < 0) return -1;
	
#ifdef SO_REUSEPORT
	int port = (reuse & XBS_REUSE_PORT) ? 1 : 0;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &port, sizeof(port)) < 0) return -1;
#endif /* SO_REUSEPORT */

	return 0; 
}

int XbsSetLinger(int fd, int onoff, int seconds)
{
	struct linger lgr = { onoff, seconds };
	return setsockopt(fd, SOL_SOCKET, SO_LINGER, (void *)&lgr, sizeof(lgr));
}

int XbsSetRbsize(int fd, int size)
{
	return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&size, sizeof(size));
}

int XbsSetWbsize(int fd, int size)
{
	return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&size, sizeof(size));
}

int XbsSetRtimeo(int fd, long timedout)
{
	struct timeval tv = { timedout / 1000, (timedout % 1000) * 1000 };
	return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv, sizeof(tv));
}

int XbsSetWtimeo(int fd, long timedout)
{
	struct timeval tv = { timedout / 1000, (timedout % 1000) * 1000 };
	return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (void *)&tv, sizeof(tv));
}

int XbsSetError(int fd, int err)
{ 
	errno = ENOPROTOOPT;
	return -1;
}

int XbsGetFlags (int fd)
{
	return fcntl(fd, F_GETFD);
}

/* first bit means reuse-port
 * second bit means reuse-addr
**/
int XbsGetReuse (int fd)
{
	int address, port = 0;
	socklen_t addrlen = sizeof(address);

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &address, addrlen) < 0) return -1;

#ifdef SOL_REUSEPORT
	socklen_t plen = sizeof(port);
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &port, plen) < 0) return -1;
#endif /* SOL_REUSEPORT */

	return (port ? XBS_REUSE_PORT : 0) | (address ? XBS_REUSE_ADDR : 0);
}

int XbsGetLinger(int fd, int *onoff, int *linger)
{ return 0; }
int XbsGetRbsize(int fd)
{ return 0; }
int XbsGetWbsize(int fd)
{ return 0; }
int XbsGetRtimeo(int fd)
{ return 0; }
int XbsGetWtimeo(int fd)
{ return 0; }

int XbsGetError(int fd) 
{
	int err;
	socklen_t elen = sizeof(err);

	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen) < 0) return -1;
	return err;
}

/* create wrapper: server & client */
int XbsServer(const char *address, int backlog, int flags, int reuse)
{
	struct sockaddr_storage addr_buf;
	socklen_t addr_len = sizeof(addr_buf);
	int type;
	int fd;

	if (str2sockaddr(address, &type, (struct sockaddr *)&addr_buf, &addr_len, 1) < 0) goto error_out;
	if ((fd = XbsSocket(addr_buf.ss_family, type, 0)) < 0) goto error_out;
	if (flags && XbsSetFlags(fd, flags, 0) < 0) goto error_close;
	if (XbsSetReuse(fd, reuse) < 0) goto error_close;
	if (bind(fd, (struct sockaddr *)&addr_buf, addr_len) < 0) goto error_close;
	if (type == SOCK_STREAM && listen(fd, backlog) < 0) goto error_close;
	return fd;
error_close:
	close(fd);
error_out:
	return -1;
}

/* create a client socket
**/
int XbsClient(const char *address, int flags, long timedout)
{
	struct sockaddr_storage addr_buf;
	socklen_t addr_len = sizeof(addr_buf);
	int type;
	int fd;

	if (str2sockaddr(address, &type, (struct sockaddr *)&addr_buf, &addr_len, 1) < 1) goto error_out;
	if ((fd = XbsSocket(addr_buf.ss_family, type, 0)) < 0) goto error_out;
	if (flags && XbsSetFlags(fd, flags, 0) < 0) goto error_close;
	if (XbsConnect(fd, (struct sockaddr *)&addr_buf, addr_len, timedout) < 0) goto error_close;
	return fd;
error_close:
	close(fd);
error_out:
	return -1;
}

}; /* namespace beyondy */

