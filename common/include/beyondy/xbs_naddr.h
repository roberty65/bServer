/* xbs_naddr.h
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
 * sock-address string <--> struct.
 *
 * address string format as:
 *    tcp://host:port
 *    udp://host:port
 *    tcp:@path:port
 *    udp:@path:port
 *
 * 1, beyondy.u.c.w, feb 3, 2008
 *    create first. 
**/
#ifndef __BEYONDY__XBS_NADDR__H
#define __BEYONDY__XBS_NADDR__H

#include <sys/types.h>
#include <sys/socket.h>

namespace beyondy {

int str2sockaddr(const char *str, int *ptype, struct sockaddr *naddr, socklen_t *nlen, int count);
int sockaddr2str(const struct sockaddr *naddr, int type, char *address, size_t addrlen);

}; /* namespace beyondy */

#endif /* __BEYONDY__XBS_NADDR__H */

