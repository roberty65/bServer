/* xbs_naddr.c
 * Copyright by beyondy, 2008-2010
 * All right reserved.
 *
**/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

namespace beyondy {

static int __unix_str2sockaddr(const char *host, struct sockaddr *naddr, socklen_t *nlen, int count)
{
	struct sockaddr_un *unaddr = (struct sockaddr_un *)&naddr[0];
	nlen[0] = sizeof(*unaddr);

	unaddr->sun_family = AF_UNIX;
	strncpy(unaddr->sun_path, host, sizeof unaddr->sun_path);
	unaddr->sun_path[sizeof unaddr->sun_path - 1] = '\0';

	return 1;
}

static int __inet_str2sockaddr(int type, const char *host, const char *serv, struct sockaddr *naddr, socklen_t *nlen, int count)
{
	struct addrinfo hints;
	struct addrinfo *result = NULL, *next;
	int i;

	memset((void *)&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;	/* Allow IPv4 or IPv6 */
	hints.ai_socktype = type;	/* TCP/UDP */
	hints.ai_flags = AI_PASSIVE;	/* For wildcard IP address */
	hints.ai_protocol = 0;		/* Any protocol */

	if (strcmp(host, "*") == 0)
		host = NULL;
	if ((errno = getaddrinfo(host, serv, &hints, &result)) != 0)
		return -1;

	next = result;
	for (i = 0; i < count && next != NULL; ++i) {
		if (next->ai_socktype != type)
			continue;	/* should not have such case */	
		if (next->ai_family == AF_INET || next->ai_family == AF_INET6) {
			memcpy(&naddr[i], next->ai_addr, next->ai_addrlen);
			nlen[i] = next->ai_addrlen;
		}

		next = next->ai_next;
	}	

	freeaddrinfo(result);
	return i;
}

int str2sockaddr(const char *str, int *ptype, struct sockaddr *naddr, socklen_t *nlen, int count)
{
	int type = SOCK_STREAM;
	if (strncmp(str, "tcp://", 6) == 0) { type = SOCK_STREAM; str += 6; }
	else if (strncmp(str, "udp://", 6) == 0) { type = SOCK_DGRAM; str += 6; }


	if (ptype != NULL) *ptype = type;
	if (*str == '@') {
		return __unix_str2sockaddr(str + 1, naddr, nlen, count);
	}

	const char *iptr;
	char host[NI_MAXHOST], serv[NI_MAXSERV];
	if ((iptr = strrchr(str, ':')) == NULL)
		return errno = EINVAL, -1;

	if (iptr - str > (int)sizeof(host) - 1)
		return errno = E2BIG, -1;
	memcpy(host, str, sizeof host);	host[iptr - str] = 0;

	int slen = str + strlen(str) - iptr - 1;
	if (slen > (int)sizeof(serv) - 1) return errno = E2BIG, -1;
	memcpy(serv, iptr + 1, slen); serv[slen] = 0;

	return __inet_str2sockaddr(type, host, serv, naddr, nlen, count);	
}


static int __unix_naddr2str(const struct sockaddr_un *unaddr, int type, char *address, size_t addrlen)
{
	int slen = snprintf(address, addrlen, "%s@%s",
			type == SOCK_STREAM ? "tcp://" : "udp://",
			unaddr->sun_path);
	if ((slen < 0 || slen >= (int)addrlen) && addrlen > 0) address[addrlen - 1] = '\0';
	return 0;
}

static int __inet_naddr2str(const struct sockaddr_in *inaddr, int type, char *address, size_t addrlen)
{
	char ibuf[INET_ADDRSTRLEN];
	int slen;

	inet_ntop(inaddr->sin_family, (void *)&inaddr->sin_addr, ibuf, sizeof(ibuf));
	slen = snprintf(address, addrlen, "%s%s:%d",
			type == SOCK_STREAM ? "tcp://" : "udp://",
			ibuf, ntohs(inaddr->sin_port));
	if ((slen < 0 || slen >= (int)addrlen) && addrlen > 0) address[addrlen - 1] = 0;	
	return 0;
}
			
static int __inet6_naddr2str(const struct sockaddr_in6 *inaddr, int type, char *address, size_t addrlen)
{
	char ibuf[INET_ADDRSTRLEN];
	int slen;

	inet_ntop(inaddr->sin6_family, (void *)&inaddr->sin6_addr, ibuf, sizeof(ibuf));
	slen = snprintf(address, addrlen, "%s%s:%d",
			type == SOCK_STREAM ? "tcp://" : "udp://",
			ibuf, ntohs(inaddr->sin6_port));
	if ((slen < 0 || slen >= (int)addrlen) && addrlen > 0) address[addrlen - 1] = 0;	
	return 0;
}

int sockaddr2str(const struct sockaddr *naddr, int type, char *address, size_t addrlen)
{
	if (naddr->sa_family == PF_UNIX || naddr->sa_family == PF_LOCAL) {
		return __unix_naddr2str((struct sockaddr_un *)naddr, type, address, addrlen);
	}
	else if (naddr->sa_family == PF_INET) {
		return __inet_naddr2str((struct sockaddr_in *)naddr, type, address, addrlen);
	}
	else if (naddr->sa_family == PF_INET6) {
		return __inet6_naddr2str((struct sockaddr_in6 *)naddr, type, address, addrlen);
	}

	errno = EAFNOSUPPORT;
	return -1;
}

}; /* namespace beyondy */

