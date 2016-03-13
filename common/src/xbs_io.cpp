/* xbs_io.cpp
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
 *
 *
 * 1, jan 25, 2008
 *    created by beyondy.
**/
#include <sys/poll.h>
#include <sys/uio.h>
#include <errno.h>
#include <beyondy/xbs_io.h>
#include <beyondy/timedout_countdown.hpp>

namespace beyondy {

/* poll */
int XbsWaitReadable(int fd, long msTimedout)
{
	int event = XbsWaitPollable(fd, POLLIN, msTimedout);
	return (event == -1) ? -1 : 0;
}

int XbsWaitWritable(int fd, long msTimedout)
{
	int event = XbsWaitPollable(fd, POLLOUT, msTimedout);
	return (event == -1) ? -1 : 0;
}

int XbsWaitPollable(int fd, short events, long msTimedout)
{
	struct pollfd pfd = { fd, events, 0 };
	int nfd = poll(&pfd, 1, msTimedout);

	if (nfd < 0) return -1;
	if (nfd == 0) return errno = ETIMEDOUT, -1;
	return pfd.revents;
}

/* send */
ssize_t XbsWrite(int fd, const void *buf, size_t size, long msTimedout)
{
	if (XbsWaitWritable(fd, msTimedout) < 0) return -1;
	return write(fd, buf, size);
}

ssize_t XbsWriteN(int fd, const void *buf, size_t size, long msTimedout)
{
	if (size == 0) return 0;

	TimedoutCountdown toCountdown(msTimedout);
	size_t sed = 0;
	ssize_t slen;

	do {
		if (XbsWaitWritable(fd, toCountdown.Update()) < 0) goto out;
		if ((slen = write(fd, (char *)buf + sed, size - sed)) < 0) goto out;
		sed += static_cast<size_t>(slen);
	} while (sed < size);
out:
	return (sed > 0) ? static_cast<ssize_t>(sed) : -1;
}

ssize_t XbsWriteV(int fd, const struct iovec *iov, int iovcnt, long msTimedout)
{
	if (XbsWaitWritable(fd, msTimedout) < 0) return -1;
	return writev(fd, iov, iovcnt);
}

ssize_t XbsWriteVN(int fd, const struct iovec *iov, int iovcnt, long msTimedout)
{
	TimedoutCountdown toCountdown(msTimedout);
	size_t sed;
	ssize_t slen;

	sed = XbsWriteV(fd, iov, iovcnt, toCountdown.Update());
	if (sed < 0) return -1;

	int i = 0;
	size_t left = sed;
	while (i < iovcnt) {
		if (iov[i].iov_len > left) {
			break;
		}
		else {
			left -= iov[i].iov_len;
		}

		++i;
	}

	for (const struct iovec *next = &iov[i]; next != &iov[iovcnt]; ++next) {
		slen = XbsWriteN(fd, (char *)next->iov_base + left, next->iov_len - left, toCountdown.Update());
		if (slen < 0) break;
		if (static_cast<size_t>(slen) < (next->iov_len - left)) break;
		sed += static_cast<size_t>(slen);
		left = 0;
	}

	return (sed > 0) ? static_cast<ssize_t>(sed) : -1;
}

/* recv */
ssize_t XbsRead(int fd, void *buf, size_t size, long msTimedout)
{
	if (XbsWaitReadable(fd, msTimedout) < 0) return -1;
	return read(fd, buf, size);
}

ssize_t XbsReadN(int fd, void *buf, size_t size, long msTimedout)
{
	TimedoutCountdown toCountdown(msTimedout);
	ssize_t red = 0, rlen = -1;

	do {
		if (XbsWaitReadable(fd, toCountdown.Update()) < 0) goto out;
		if ((rlen = read(fd, (char *)buf + red, size - red)) < 1) goto out;
		red += rlen;
	} while (red < (ssize_t)size);
out:
	if (red > 0) {
		return red;
	}
	else if (rlen == 0) {
		return 0;
	}

	return -1;
}

ssize_t XbsReadV(int fd, struct iovec *iov, int iovcnt, long msTimedout)
{ return 0; }

ssize_t XbsReadVN(int fd, struct iovec *iov, int iovcnt, long msTimedout)
{ return 0; }

}; /* namespace beyondy */

