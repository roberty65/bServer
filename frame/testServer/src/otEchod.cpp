#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "beyondy/xbs_socket.h"


static int events_watch_add(int efd, int fd, int events)
{
        struct epoll_event ee;
        ee.events = events; ee.data.fd = fd;
        
        if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ee) < 0) {
                fprintf(stderr, "epoll_ctl(ADD, %d) failed: %m\n", fd);
                return -1;
        }

        return 0;
}

static int events_watch_del(int efd, int fd)
{
        struct epoll_event ee;
        ee.events = 0; ee.data.fd = fd;

        if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, &ee) < 0) {
                fprintf(stderr, "epoll_ctl(DEL, %d) failed: %m\n", fd);
                return -1;
        }

        return 0;
}

static int handle_events(int fd, int events)
{
	char buf[8192];
	ssize_t rlen = read(fd, buf, sizeof buf);
	if (rlen > 0) {
		ssize_t wlen = write(fd, buf, rlen);
		if (wlen != rlen) {
			fprintf(stderr, "write %d failed: rlen=%ld, wlen=%ld err: %m\n", fd, (long)rlen, (long)wlen);
			return -1;
		}
	}
	else {
		if (rlen == 0) fprintf(stderr, "socket %d is closed by peer\n", fd);
		else fprintf(stderr, "socket %d read error: %m\n", fd);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
        int fd, efd;

        if (argc < 2) {
                fprintf(stderr, "Usage: %s address\n", argv[0]);
                return 0;
        }

        if ((fd = beyondy::XbsServer(argv[1], 1024, O_NONBLOCK)) < 0) {
                fprintf(stderr, "xbsServer failed: %m\n");
                return 1;
        }

        if ((efd = epoll_create(1024)) < 0) {
                fprintf(stderr, "epoll_create failed: %m\n");
                return 2;
        }

        int epoll_size = 1024;
        struct epoll_event *epoll_events = (struct epoll_event *)malloc(epoll_size * sizeof(struct epoll_event));
        if (epoll_events == NULL) {
                fprintf(stderr, "new epoll events failed: %m\n");
                return 3;
        }

        if (events_watch_add(efd, fd, EPOLLIN) < 0)
		return 4;

        while (true) {
                long wtime = 1000; /* 1s */
                int cnt = epoll_wait(efd, epoll_events, epoll_size, wtime);
                if (cnt > 0) for (int i = 0; i < cnt; ++i) {
                        struct epoll_event *ee = &epoll_events[i];
                        if (ee->data.fd == fd) {
                                int cfd = accept(fd, NULL, NULL);
                                if (cfd >= 0) events_watch_add(efd, cfd, EPOLLIN);
                        }
                        else if (handle_events(ee->data.fd, ee->events) < 0) {
				events_watch_del(efd, ee->data.fd);
				close(ee->data.fd);
                        }
                }
        }

        return 0;
}

