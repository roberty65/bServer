#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>

#include <beyondy/xbs_socket.h>
#include <beyondy/xbs_io.h>

struct proto_h16_head {
	uint32_t len;
	uint16_t cmd;
	uint16_t ver;
	uint32_t syn;
	uint32_t ack;
};

struct proto_h16_res : public proto_h16_head {
	int32_t ret_;
};

#define MAX_PSIZE	8100
#define BUF_SIZE	8192

static const char* host;
static long tcnt, rcnt, psize;
static long idx = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static long next()
{
	pthread_mutex_lock(&m);
	long nxt = idx++;
	pthread_mutex_unlock(&m);
	return nxt;
}

static void *thread_entry(void *p)
{
	int fd = beyondy::XbsClient(host, 0 /* O_NONBLOCK */, 30000);
	if (fd < 0) {
		perror("connect to host failed");
		exit(1);
	}
	
	char buf[BUF_SIZE], rbuf[BUF_SIZE];
	struct proto_h16_head h;
	struct proto_h16_res r;
	long nxt;

	while ((nxt = next()) < rcnt) {
		h.len = sizeof h + psize;
		h.cmd = 0;
		h.ver = 1;
		h.syn = nxt;
		h.ack = 0;
		memcpy(buf, &h, sizeof h);
		//for (int i = 0; i < psize; ++i) buf[sizeof h + i] = i;

		ssize_t wlen = beyondy::XbsWriteN(fd, buf, h.len, 10000);
		if (wlen != h.len) {
			perror("write error");
			exit(1);
		}

		ssize_t rlen = beyondy::XbsReadN(fd, rbuf, wlen, 10000);
		if ((size_t)rlen != (size_t)wlen) {
			perror("read error");
			exit(2);
		}

		memcpy(&r, rbuf, sizeof r);
		//fprintf(stdout, "rsp-size=%ld, ret=%d\n", (long)rlen, r.ret_);
	}

	close(fd);
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc != 5) {
		fprintf(stderr, "Usage: %s host thread-cnt request-cnt pkg-size\n", argv[0]);
		exit(0);
	}

	struct timeval t1, t2;
	pthread_t tids[tcnt];
	
	host = argv[1];
	tcnt = atol(argv[2]);
	rcnt = atol(argv[3]);
	psize = atol(argv[4]);

	gettimeofday(&t1, NULL);
	for (int i = 0; i < tcnt; ++i) {
		int retval = pthread_create(&tids[i], NULL, thread_entry, NULL);
		if (retval != 0) { errno = retval; perror("pthread_create failed"); exit(1); }
	}

	for (int i = 0; i < tcnt; ++i) {
		pthread_join(tids[i], NULL);
	}

	gettimeofday(&t2, NULL);
	double ms = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
	if (ms < 1.0) ms = 1.0;
	fprintf(stdout, "%ld threads handled %ld requests sized %ld in %5.3fms. Performance is %5.3f#/s.\n",
		tcnt, rcnt, psize, ms, rcnt * 1000.0 / ms);
	
	return 0;
}
