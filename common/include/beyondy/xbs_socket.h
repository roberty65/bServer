/* bsocket.h
 * Copyright by beyondy, 2004-2010
 * All rights reserved.
 *
**/

#ifndef __BEYONDY__XBS_SOCKET__H
#define __BEYONDY__XBS_SOCKET__H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>

namespace beyondy {

#define XBS_REUSE_ADDR	0x01
#define XBS_REUSE_PORT	0x02

/* base wrapper */
int XbsSocket(int domain, int type, int protocol);
int XbsBind(int fd, const char *address);
int XbsListen(int fd, int backlog);
int XbsAccept(int fd, long timedout);
int XbsConnect(int fd, const struct sockaddr *target, socklen_t alen, long msTimedout = -1);
int XbsConnect(int fd, const char *address, long msTimedout = -1);

/* get peer & local name */
int XbsGetPeername(int fd, char *nbuf, size_t size);
int XbsGetLocalname(int fd, char *nbuf, size_t size);

/* flags & other socket ops */
int XbsSetFlags(int fd, int on, int off);
int XbsSetReuse(int fd, int reuse);
int XbsSetLinger(int fd, int onoff, int linger);
int XbsSetRbsize(int fd, int size);
int XbsSetWbsize(int fd, int size);
int XbsSetRtimeo(int fd, long timedout);
int XbsSetWtimeo(int fd, long timedout);
int XbsSetError(int fd, int err);

int XbsGetFlags(int fd);
int XbsGetReuse(int fd);
int XbsGetLinger(int fd, int *onoff, int *linger);
int XbsGetRbsize(int fd);
int XbsGetWbsize(int fd);
int XbsGetRtimeo(int fd);
int XbsGetWtimeo(int fd);
int XbsGetError(int fd);

/* create wrapper: server & client */
int XbsServer(const char *address, int backlog = 5, int flags = 0, int reuse = XBS_REUSE_ADDR | XBS_REUSE_PORT);
int XbsClient(const char *address, int flags = 0, long msTimedout = -1);

}; /* namespace beyondy */

#endif /*!__BEYONDY__XBS_SOCKET__H */

