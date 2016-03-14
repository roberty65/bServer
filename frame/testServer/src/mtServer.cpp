#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <beyondy/xbs_socket.h>

static void *worker(void *p)
{
	int fd = (int)(long)p;
	char buf[8192];

	while (true) {
		ssize_t rlen = ::read(fd, buf, sizeof buf);
		if (rlen < 0) {
			if (errno == EAGAIN) continue;
			fprintf(stderr, "socket %d read failed: %m\n", fd);
			break;
		}
		else if (rlen == 0) {
			fprintf(stderr, "socket %d is closed by peer\n", fd);
			break;
		}	

		ssize_t wlen = ::write(fd, buf, rlen);
		if (wlen != rlen) {
			fprintf(stderr, "socket %d write failed: %m\n", fd);
			break;
		}
	}

	close(fd);
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s host\n", argv[0]);
		exit(1);
	}
	
	const char *host = argv[1];
	int cfd, fd = beyondy::XbsServer(host, 1024);

	if (fd < 0) {
		fprintf(stderr, "listen at %s failed: %m\n", host);
		exit(2);
	}

	while (true) {
		if ((cfd = ::accept(fd, NULL, NULL)) < 0)
			continue;
		pthread_t tid;
		errno = pthread_create(&tid, NULL, worker, (void *)(long)cfd);
		if (errno) {
			fprintf(stderr, "pthread-create failed: %m\n");
			close(cfd);
		}
	}

	return 0;
}
