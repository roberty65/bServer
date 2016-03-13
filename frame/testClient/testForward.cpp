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

static int sizes[] = { 0, 16, 64, 128/*, KK(1), KK(8), KK(64), KK(256), KK(512), MM(1), MM(4), MM(8), MM(10), MM(20)*/ };
static int scnt = (int)(sizeof(sizes)/sizeof(sizes[0]));
static int cmd = 1;	// 0: echo, 1: forward

static const char *host = "inet@127.0.0.1:5010/tcp";
static int timeout = 5000;

static int __send_req(int fd, int cmd, int size, const char *tag)
{
	struct proto_h16_head *h; 
	int msize = size + sizeof(*h);

	char *reqbuf = new char[msize];	
	h = (struct proto_h16_head *)reqbuf;

	h->len = msize;
	h->cmd = cmd;
	h->ver = 1;
	h->syn = 2;
	h->ack = 3;

	if (size >= (int)sizeof(int)) {
		*(int *)(reqbuf + sizeof *h) = random() % 3000;
	}	

	ssize_t wlen = beyondy::XbsWriteN(fd, reqbuf, msize, timeout);
	if (wlen != msize) {
		fprintf(stdout, "FAIL: %s pkg-size=%d, write req failed: %m\n", tag, size);
		return -1;	
	}

	delete[] reqbuf;	
	return 0;
}

static int __read_rsp(int fd, int size, const char *tag)
{
	int msize = size + sizeof(struct proto_h16_head);
	char *rspbuf = new char[msize];

	ssize_t rlen = beyondy::XbsReadN(fd, rspbuf, msize, timeout);
	if (rlen != msize) {
		fprintf(stdout, "FAIL: %s pkg-size=%d, read rsp failed: %m\n", tag, msize);
		return -1;	
	}

	// TODO: cksum rspbuf
	delete[] rspbuf;
	return 0;
}

static int __test001_sync_req_rsp(int fd, int size)
{
	static const char *tag = "test001: sync-req-rsp";
	struct timeval t1, t2, t3;

	gettimeofday(&t1, NULL);
	if (__send_req(fd, cmd, size, tag) < 0) return -1;

	gettimeofday(&t2, NULL);
	if (__read_rsp(fd, size, tag) < 0) return -1;

	gettimeofday(&t3, NULL);
	fprintf(stdout, "OK: %s pkg-size=%d, write-time=%ldms read-rsp=%ldms\n",
		tag, size, DIFF_TV(&t1, &t2), DIFF_TV(&t2, &t3));
	return 0;
}

static void test001_sync_req_rsp()
{
	static const char *tag = "test001: sync-req-rsp";
	fprintf(stdout, "%s start...\n", tag);
	int fd = -1;
	
	for (int i = 0; /* ** */; ++i) {
		int j = i % scnt;
		if (fd < 0) {
			if ((fd = beyondy::XbsClient(host, O_NONBLOCK, 30000)) < 0) {
				fprintf(stdout, "%s connect failed: %m\n", tag);	
				sleep(1);
				continue;
			}
			else {
				fprintf(stdout, "connect OK\n");
			}
		}

		if (__test001_sync_req_rsp(fd, sizes[j]) < 0) {
			close(fd); fd = -1;
			sleep(1);
		}
	}

	close(fd);
	fprintf(stdout, "%s done\n", tag);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s host [cmd]\n", argv[0]);
		exit(0);
	}

	srandom(time(NULL));	
	setbuf(stdout, NULL);

	host = argv[1];
	if (argc >= 3) cmd = atoi(argv[2]);

	test001_sync_req_rsp();

	return 0;
}
