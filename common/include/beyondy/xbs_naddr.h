/* xbs_naddr.h
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
 * sock-address string <--> struct.
 *
 * address string format as:
 *    [domain@]address[:service]
 *  1, domain can be number as PF_UNIX, etc. or name as unix/inet/inet6, etc.
 *     if not specified, which will be guessed by address.
 *  2, address can be sorrounded by "[]" optionally.
 *  3, when domain is unix, address is the unix path.
 *     service is numeric or name of stream/tcp, dgram/udp, etc
 *  3, when domain is inet/inet6, address is the ip-addr(dotted-quad format or host-name)
 *     service is service-name or port/type
 *  4, if domain is ommited, which can be guessed by the following way:
 *     it's unix if:
 *        started by '/'
 *     it's inet if:
 *        dotted-quad format
 *     it's inet6 if:
 * 	  ipv6-formated address
 *
 * 1, beyondy.u.c.w, feb 3, 2008
 *    create first. 
**/
#ifndef __BEYONDY__XBS_NADDR__H
#define __BEYONDY__XBS_NADDR__H

#include <sys/types.h>
#include <sys/socket.h>

namespace beyondy {

int XbsPaddr2n(const char *address, int *type, int *protocol, struct sockaddr *naddr, socklen_t *nlen, size_t* pos = NULL);
int XbsNaddr2p(const struct sockaddr *naddr, int type, int protocol, char *address, size_t addrlen);

}; /* namespace beyondy */

#endif /* __BEYONDY__XBS_NADDR__H */

