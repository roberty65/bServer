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
#include <beyondy/nam2val.h>

namespace beyondy {

static const struct nam_val_struct domain_maps[] = { 
	{ "unix", 	PF_UNIX  },
	{ "local",	PF_LOCAL },
	{ "inet",	PF_INET  },
	{ "inet6",	PF_INET6 },
	{ NULL, 	-1       }
};

static const struct nam_val_struct type_maps[] = {
	{ "tcp",	SOCK_STREAM },
	{ "stream",	SOCK_STREAM },
	{ "udp",	SOCK_DGRAM  },
	{ "dgram",	SOCK_DGRAM  },
	{ NULL, 	-1          }
};

static int __unix_p2naddr(const char *address, int *ptype, int *pprotocol, 
			  struct sockaddr_un *unaddr, socklen_t *nlen)
{
	const char *sptr;
	int type;

	if (nlen == NULL || *nlen < sizeof(*unaddr)) {
		errno = EINVAL;
		return -1;
	}

	if ((sptr = strrchr(address, ':')) == NULL) {
		type = SOCK_STREAM;
		sptr = address + strlen(address);
	}
	else if ((type = nam2val(type_maps, (sptr[1] == '/') ? sptr + 2 : sptr + 1, 0)) == -1) {
		errno = EINVAL;
		return -1;
	}

	if ((int)(sptr - address) > (int)(sizeof(unaddr->sun_path) - 1)) {
		errno = E2BIG;
		return -1;
	}

	if (*ptype) *ptype = type;
	if (*pprotocol) *pprotocol = 0;	// default. todo

	unaddr->sun_family = PF_UNIX;
	memcpy(unaddr->sun_path, address, (size_t)(sptr - address));
	unaddr->sun_path[(int)(sptr - address)] = '\0';

	*nlen = sizeof(*unaddr);

	return 0;
}

/* dot numbers or dns to inet-addr
 * target point to sockaddr
**/
static int __inetx_p2n(int af, const char *address, int *ptype, int *pprotocol, void *target, size_t* pos)
{
	struct addrinfo hint;
	struct addrinfo *result = NULL, *next;
	const char *sptr, *tptr;
	char host[NI_MAXHOST], serv[NI_MAXSERV];
	int retval;

#define TRUNC_S_COPY_N(d, s, n, max) do {				\
		size_t __n = (n);					\
		if (__n >= (max)) __n = (max) - 1; 			\
		memcpy((d), (s), __n);					\
		*((char *)(d) + __n) = '\0';				\
	} while (0)

#define TRUNC_S_COPY(d, s, max) do { 					\
		memcpy((d), (s), (max) - 1);				\
		*((char *)(d) + (max) - 1) = '\0';			\
	} while (0)

	memset((void *)&hint, 0, sizeof(hint));
	hint.ai_family = af;

	sptr = strrchr(address, ':');
	if (sptr == NULL) {
		hint.ai_flags |= AI_NUMERICSERV;
		hint.ai_socktype = SOCK_STREAM;

		TRUNC_S_COPY(host, address, sizeof(host));
		snprintf(serv, sizeof(serv), "%d", 0);
	}
	else if ((tptr = strchr(sptr + 1, '/')) != NULL) {
		// format as port/type
		hint.ai_flags |= AI_NUMERICSERV;
		if (tptr[1] == '\0' || (tptr[1] == '*' && tptr[2] == '\0')) {
			hint.ai_socktype = 0;
		}
		else if ((hint.ai_socktype = nam2val(type_maps, tptr + 1, 0)) == -1) {
			errno = EINVAL;
			return -1;
		}

		TRUNC_S_COPY_N(host, address, size_t(sptr - address), sizeof(host));
		TRUNC_S_COPY_N(serv, sptr + 1, size_t(tptr - sptr - 1), sizeof(serv));
	}
	else {
		// format as port, default is tcp
		hint.ai_flags |= AI_NUMERICSERV;
		hint.ai_socktype = SOCK_STREAM;

		TRUNC_S_COPY_N(host, address, size_t(sptr - address), sizeof(host));
		TRUNC_S_COPY(serv, sptr + 1, sizeof(serv));
	}

#undef TRUNC_S_COPY_N
#undef TRUNC_S_COPY

	if ((retval = getaddrinfo(host, serv, &hint, &result)) < 0) {
		// No such case?
		return -1;
	}
	else if (retval != 0) {
		errno = retval;
		return -1;
	}

	next = result;
	for (size_t i = 0; i < (pos == NULL ? 0 : *pos) && next != NULL; ++i) {
		next = next->ai_next;
	}

	if (next != NULL) {
		if (ptype) *ptype = next->ai_socktype;
		if (pprotocol) *pprotocol = next->ai_protocol;
		memcpy(target, next->ai_addr, next->ai_addrlen);

		if (pos != NULL) ++*pos;
		retval = 0;
	}
	else {
		errno = ENOENT;
		retval = -1;
	}

	freeaddrinfo(result);
	return retval;
}

static int __inet_p2naddr(const char *address, int *ptype, int *pprotocol, 
			  struct sockaddr_in *inaddr, socklen_t *nlen, size_t* pos)
{
	if (nlen == NULL || *nlen < sizeof(*inaddr)) {
		errno = EINVAL;
		return -1;
	}

	if (__inetx_p2n(PF_INET, address, ptype, pprotocol, inaddr, pos) < 0) return -1;

	inaddr->sin_family = PF_INET;
	*nlen = sizeof(*inaddr);

	return 0;
}

static int __inet6_p2naddr(const char *address, int *ptype, int *pprotocol, 
			   struct sockaddr_in6 *inaddr, socklen_t *nlen, size_t* pos)
{
	if (nlen == NULL || *nlen < sizeof(*inaddr)) {
		errno = EINVAL;
		return -1;
	}

	if (__inetx_p2n(PF_INET6, address, ptype, pprotocol, inaddr, pos) < 0) return -1;

	inaddr->sin6_family = PF_INET6;
	*nlen = sizeof(*inaddr);

	return 0;
}

static int __guess_domain(const char *address)
{
	struct sockaddr_storage addr_buf;
	const char *aptr = address, *eptr;
	char host[NI_MAXHOST];

	if (*aptr == '[' && (eptr = strrchr(aptr + 1, ']')) != NULL) {
		++aptr;
	}
	else if ((eptr = strrchr(aptr, ':')) == NULL) {
		eptr = address + strlen(address);
	}

	if (*aptr == '/') return PF_UNIX;

	if (size_t(eptr - aptr) > (sizeof(host) - 1)) {
		errno = E2BIG;
		return -1;
	}

	strncpy(host, aptr, eptr - aptr);
	host[eptr - aptr] = '\0';

	if (inet_pton(AF_INET, host, &addr_buf) > 0) return PF_INET;
	if (inet_pton(AF_INET6, host, &addr_buf) > 0) return PF_INET6;
	return PF_UNSPEC;
}

int XbsPaddr2n( const char *address, int *ptype, int *pprotocol, 
		struct sockaddr *naddr, socklen_t *nlen, size_t* pos)
{
	int domain;
	const char *aptr;

	if (address == NULL) {
		errno = EINVAL;
		return -1;
	}

	memset(naddr, 0, *nlen);
	aptr= strchr(address, '@');
	if (aptr == NULL) {
		aptr = address;
		if ((domain = __guess_domain(address)) < 0) return -1;
	}
	else {
		if ((domain = nam2val(domain_maps, address, (int)(aptr - address))) == -1) {
			errno = EAFNOSUPPORT;
			goto error_out;
		}

		++aptr;
	}

	if (domain == PF_UNIX || domain == PF_LOCAL) {
		if (__unix_p2naddr(aptr, ptype, pprotocol, (struct sockaddr_un *)naddr, nlen) < 0)
			goto error_out;
		// good, fall through ...
	}
	else if (domain == PF_INET) {
		if (__inet_p2naddr(aptr, ptype, pprotocol, (struct sockaddr_in *)naddr, nlen, pos) < 0)
			goto error_out;
		// good, fall through ...
	}
	else if (domain == PF_INET6) {
		if (__inet6_p2naddr(aptr, ptype, pprotocol, (struct sockaddr_in6 *)naddr, nlen, pos) < 0)
			goto error_out;
		// good, fall through ...
	}
	else {
		errno = EAFNOSUPPORT;
		goto error_out;
	}	

	return 0;
error_out:
	return -1;
}

static int __unix_naddr2p(const struct sockaddr_un *unaddr, int type, int protocol, 
			  char *address, size_t addrlen)
{
	int slen = snprintf(address, addrlen, "%s@%s:%s",
			val2nam(domain_maps, unaddr->sun_family),
			unaddr->sun_path,
			val2nam(type_maps, type));
	if ((slen < 0 || slen >= (int)addrlen) && addrlen > 0) address[addrlen - 1] = '\0';
	return 0;
}

static int __inet_naddr2p(const struct sockaddr_in *inaddr, int type, int protocol, 
			  char *address, size_t addrlen)
{
	char ibuf[INET_ADDRSTRLEN];
	char nbuf[64];
	const char *tname = val2nam(type_maps, type);
	int slen;

	inet_ntop(inaddr->sin_family, (void *)&inaddr->sin_addr, ibuf, sizeof(ibuf));

	if (tname == NULL) {
		snprintf(nbuf, sizeof(nbuf), "[%d]", type);
		tname = nbuf;
	}

	slen = snprintf(address, addrlen, "%s@%s:%d/%s",
			val2nam(domain_maps, inaddr->sin_family),
			ibuf, ntohs(inaddr->sin_port),
			tname);
	if ((slen < 0 || slen >= (int)addrlen) && addrlen > 0) address[addrlen - 1] = 0;	
	return 0;
}
			
static int __inet6_naddr2p(const struct sockaddr_in6 *inaddr, int type, int protocol, 
			   char *address, size_t addrlen)
{
	char ibuf[INET_ADDRSTRLEN];
	char nbuf[64];
	const char *tname = val2nam(type_maps, type);
	int slen;

	inet_ntop(inaddr->sin6_family, (void *)&inaddr->sin6_addr, ibuf, sizeof(ibuf));

	if (tname == NULL) {
		snprintf(nbuf, sizeof(nbuf), "[%d]", type);
		tname = nbuf;
	}

	slen = snprintf(address, addrlen, "%s@%s:%d/%s",
			val2nam(domain_maps, inaddr->sin6_family),
			ibuf, ntohs(inaddr->sin6_port),
			tname);
	if ((slen < 0 || slen >= (int)addrlen) && addrlen > 0) address[addrlen - 1] = 0;	
	return 0;
}

int XbsNaddr2p(const struct sockaddr *naddr, int type, int protocol, char *address, size_t addrlen)
{
	if (naddr->sa_family == PF_UNIX || naddr->sa_family == PF_LOCAL) {
		return __unix_naddr2p((struct sockaddr_un *)naddr, type, protocol, address, addrlen);
	}
	else if (naddr->sa_family == PF_INET) {
		return __inet_naddr2p((struct sockaddr_in *)naddr, type, protocol, address, addrlen);
	}
	else if (naddr->sa_family == PF_INET6) {
		return __inet6_naddr2p((struct sockaddr_in6 *)naddr, type, protocol, address, addrlen);
	}

	errno = EAFNOSUPPORT;
	return -1;
}

}; /* namespace beyondy */

