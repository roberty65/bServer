/* xbs_io.h
 * Copyright by beyondy, 2008-2010
 * All rights reserved.
 *
 * 1, beyondy, jan 13, 2008
 *    created.
**/
#ifndef __BEYONDY__XBS_IO__H
#define __BEYONDY__XBS_IO__H

#include <sys/types.h>
#include <unistd.h>

namespace beyondy {

/* poll */
int XbsWaitReadable(int fd, long msTimedout = -1);
int XbsWaitWritable(int fd, long msTimedout = -1);
int XbsWaitPollable(int fd, short events, long msTimedout = -1);

/* send */
ssize_t XbsWrite(int fd, const void *buf, size_t size, long msTimedout = -1);
ssize_t XbsWriteN(int fd, const void *buf, size_t size, long msTimedout = -1);
ssize_t XbsWriteV(int fd, const struct iovec *iov, int iovcnt, long msTimedout = -1);
ssize_t XbsWriteVN(int fd, const struct iovec *iov, int iovcnt, long msTimedout = -1);

/* recv */
ssize_t XbsRead(int fd, void *buf, size_t size, long msTimedout = -1);
ssize_t XbsReadN(int fd, void *buf, size_t size, long msTimedout = -1);
ssize_t XbsReadV(int fd, struct iovec *iov, int iovcnt, long msTimedout = -1);
ssize_t XbsReadVN(int fd, struct iovec *iov, int iovcnt, long msTimedout = -1);

}; /*namespace beyondy */

#endif /* __BEYONDY__XBS_IO__H */
