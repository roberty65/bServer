#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
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

#define KK(n)	((n) * 1024)
#define MM(n)	((n) * 1024 * 1024)
#define GG(n)	((n) * 1024 * 1024 * 1024)

#define DIFF_TV(t1, t2)	((long)(((t2)->tv_sec - (t1)->tv_sec) * 1000 + ((t2)->tv_usec - (t1)->tv_usec) / 1000))

//static int sizes[] = { 0, 16, 64, 128, KK(1), KK(8), KK(64), KK(256), KK(512), MM(1), MM(4), MM(8), MM(10), MM(20) };
//static int scnt = (int)(sizeof(sizes)/sizeof(sizes[0]));
static int cmd = 1;	// 0: echo, 1: forward
static int tcnt = 10;
static int pkgSize = 1024;

static volatile long *countSend = NULL;
static volatile long *countRecv = NULL;

static const char *host = "inet@127.0.0.1:5010/tcp";
static int timeout = 30000;

static int __send_req(int fd, int cmd, char *reqbuf, int size, const char *tag)
{
	struct proto_h16_head *h = (struct proto_h16_head *)reqbuf;
	int msize = size + sizeof(*h);

	h->len = msize;
	h->cmd = cmd;
	h->ver = 1;
	h->syn = 2;
	h->ack = 3;

//	if (size >= (int)sizeof(int)) {
//		*(int *)(reqbuf + sizeof *h) = random() % 3000;
//	}	

	ssize_t wlen = beyondy::XbsWriteN(fd, reqbuf, msize, timeout);
	if (wlen != msize) {
		fprintf(stdout, "FAIL: %s pkg-size=%d, write req failed: %m\n", tag, size);
		return -1;	
	}

	return 0;
}

static int __read_rsp(int fd, char *rspbuf, int size, const char *tag)
{
	int msize = size + sizeof(struct proto_h16_head);
	ssize_t rlen = beyondy::XbsReadN(fd, rspbuf, msize, timeout);

	if (rlen != msize) {
		fprintf(stdout, "FAIL: %s pkg-size=%d, read rsp failed: %m\n", tag, msize);
		return -1;	
	}

	return 0;
}


static void *worker(void *p)
{
	int id = (int)(long)p;
	char tag[128]; snprintf(tag, sizeof tag, "thread-%d", id);

	char *reqbuf = new char[sizeof(struct proto_h16_head) + pkgSize];
	char *rspbuf = new char[sizeof(struct proto_h16_res) + pkgSize];

	int fd = beyondy::XbsClient(host, O_NONBLOCK, 30000);
	if (fd < 0) {
		fprintf(stdout, "%s connect failed: %m\n", tag);
		exit(1);
	}

	while (true) {	
		if (__send_req(fd, cmd, reqbuf, pkgSize, tag) < 0) {
			fprintf(stdout, "%s send req failed: %m\n", tag);
			break;
		}

		++countSend[id];
		if (__read_rsp(fd, rspbuf, pkgSize, tag) < 0) {
			fprintf(stdout, "%s read rsp failed: %m\n", tag);
			break;
		}

		++countRecv[id];
	}

	return NULL;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
usage:
		fprintf(stderr, "Usage: %s [-c cmd ] [-t thread-cnt] [-s pkg-size] host\n", argv[0]);
		exit(0);
	}

	int ch;
	while ((ch = getopt(argc, argv, "c:t:s:h")) != EOF) {
		switch (ch) {
		case 'c': cmd = atoi(optarg); break;
		case 't': tcnt = atoi(optarg); break;
		case 's': pkgSize = atoi(optarg); break;
		case 'h': goto usage;
		default: goto usage;
		}
	}

	if (optind >= argc) goto usage;
	host = strdup(argv[optind]);

	countSend = new long[tcnt];
	countRecv = new long[tcnt];
	for (int i = 0; i < tcnt; ++i) countSend[i] = countRecv[i] = 0;

	srandom(time(NULL));
	setbuf(stdout, NULL);

	pthread_t tids[tcnt];
	for (int i = 0; i < tcnt; ++i) {
		errno = pthread_create(&tids[i], NULL, worker, (void *)(long)i);
		if (errno) {
			fprintf(stdout, "create the %dth thread failed: %m\n", i);
			exit(1);
		}
	}

	long lastSend = 0, lastRecv = 0;
	struct timeval t1, t2;

	gettimeofday(&t1, NULL);
	while (true) {
		sleep(10);
		long thisSend = 0, thisRecv = 0;

		for (int i = 0; i < tcnt; ++i) {
			thisSend += countSend[i];
			thisRecv += countRecv[i];
		}

		gettimeofday(&t2, NULL);
		struct tm tmbuf, *ptm = localtime_r(&t2.tv_sec, &tmbuf);
		double ms = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;

		fprintf(stdout, "%02d:%02d:%02d send %8ld %.2fpkg/s %.2fMB/s recv %8ld %.2fpkg/s %.2fMB/s\n",
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
			thisSend - lastSend, 1000.0 * (thisSend - lastSend) / ms, 1000.0 * (thisSend - lastSend) / ms * pkgSize / 1e6,
			thisRecv - lastRecv, 1000.0 * (thisRecv - lastRecv) / ms, 1000.0 * (thisRecv - lastRecv) / ms * pkgSize / 1e6);

		t1 = t2;
		lastSend = thisSend;
		lastRecv = thisRecv;
	}

	return 0;
}
