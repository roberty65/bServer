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

static int sizes[] = { 0, 16, 64, 128, KK(1), KK(8), KK(64), KK(256), KK(512), MM(1), MM(4), MM(8), MM(10), MM(20) };
static int scnt = (int)(sizeof(sizes)/sizeof(sizes[0]));
static int tcnt = 10;

static const char *host = "inet@127.0.0.1:5010/tcp";
static int timeout = 15000;

static void __send_req(int fd, int cmd, int size, const char *tag)
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
		*(int *)(reqbuf + sizeof *h) = random() % 3;
	}	

	ssize_t wlen = beyondy::XbsWriteN(fd, reqbuf, msize, timeout);
	if (wlen != msize) {
		fprintf(stdout, "FAIL: %s pkg-size=%d, write req failed: %m\n", tag, size);
		exit(1);
	}

	delete[] reqbuf;	
	return;
}

static void __read_rsp(int fd, int size, const char *tag)
{
	int msize = size + sizeof(struct proto_h16_head);
	char *rspbuf = new char[msize];

	ssize_t rlen = beyondy::XbsReadN(fd, rspbuf, msize, timeout);
	if (rlen != msize) {
		fprintf(stdout, "FAIL: %s pkg-size=%d, read rsp failed: %m\n", tag, msize);
		exit(1);
	}

	// TODO: cksum rspbuf
	delete[] rspbuf;
	return;
}

static void __test001_sync_req_rsp(int fd, int size)
{
	static const char *tag = "test001: sync-req-rsp";
	struct timeval t1, t2, t3;

	gettimeofday(&t1, NULL);
	__send_req(fd, 1, size, tag);

	gettimeofday(&t2, NULL);
	__read_rsp(fd, size, tag);

	gettimeofday(&t3, NULL);
	fprintf(stdout, "OK: %s pkg-size=%d, write-time=%ldms read-rsp=%ldms\n",
		tag, size, DIFF_TV(&t1, &t2), DIFF_TV(&t2, &t3));
	return;
}

static void test001_sync_req_rsp()
{
	static const char *tag = "test001: sync-req-rsp";
	fprintf(stdout, "%s start...\n", tag);

	int fd = beyondy::XbsClient(host, O_NONBLOCK, 30000);
	if (fd < 0) {
		fprintf(stdout, "%s connect failed: %m\n", tag);	
		exit(1);
	}
	
	for (int i = 0; i < scnt; ++i) {
		__test001_sync_req_rsp(fd, sizes[i]);
	}

	close(fd);
	fprintf(stdout, "%s done\n", tag);
}

static void *__test002_sender_entry(void *p)
{
	static const char *tag = "test002: async-req-rsp sender";

	int fd = (int)(long)p;	
	for (int i = 0; i < scnt; ++i) {
		struct timeval t1, t2;

		gettimeofday(&t1, NULL);
		__send_req(fd, 2, sizes[i], tag);

		gettimeofday(&t2, NULL);
		fprintf(stdout, "%s send req with pkg-size=%ld took %ldms\n", \
			tag, (long)sizes[i], DIFF_TV(&t1, &t2));
	}

	return NULL;	
}

static void *__test002_reader_entry(void *p)
{
	static const char *tag = "test002: async-req-rsp reader";

	int fd = (int)(long)p;
	for (int i = 0; i < scnt; ++i) {
		struct timeval t1, t2;

		gettimeofday(&t1, NULL);
		__read_rsp(fd, sizes[i], tag);

		gettimeofday(&t2, NULL);
		fprintf(stdout, "%s read rsp for pkg-size=%ld took %ldms\n", 
			tag, (long)sizes[i], DIFF_TV(&t1, &t2));
	}

	return NULL;	
}

static void test002_async_req_rsp()
{
	static const char *tag = "test002: async-req-rsp";
	fprintf(stdout, "%s start...\n", tag);

	int fd = beyondy::XbsClient(host, O_NONBLOCK, 30000);
	if (fd < 0) {
		fprintf(stdout, "%s connect failed: %m\n", tag);
		exit(1);
	}

	pthread_t senderTid, readerTid;
	errno = pthread_create(&senderTid, NULL, __test002_sender_entry, (void *)(long)fd);
	if (errno != 0) {
		fprintf(stdout, "%s create thread for sender failed: %m\n", tag);
		exit(1);
	}

	errno = pthread_create(&readerTid, NULL, __test002_reader_entry, (void *)(long)fd);
	if (errno != 0) {
		fprintf(stdout, "%s create thread for reader failed: %m\n", tag);
		exit(1);
	}

	pthread_join(senderTid, NULL);
	pthread_join(readerTid, NULL);

	close(fd);
	fprintf(stdout, "%s done\n", tag);
}

static void *__test003_worker(void *p)
{
	static const char *tag = "test003: mt-sync-req-rsp-worker";

	int fd = beyondy::XbsClient(host, O_NONBLOCK, 30000);
	if (fd < 0) {
		fprintf(stdout, "%s connect failed: %m\n", tag);
		exit(1);
	}

	for (int i = 0; i < scnt; ++i) {
		struct timeval t1, t2, t3;

		gettimeofday(&t1, NULL);	
		__send_req(fd, 3, sizes[i], tag);

		gettimeofday(&t2, NULL);
		__read_rsp(fd, sizes[i], tag);

		gettimeofday(&t3, NULL);
		fprintf(stdout, "%s pkg-size=%ld send took %ldms, read took %ld ms\n", \
			tag, (long)sizes[i], DIFF_TV(&t1, &t2), DIFF_TV(&t2, &t3));
	}

	return NULL;
}

static void test003_mt_sync_req_rsp()
{
	static const char *tag = "test003: mt-sync-req-rsp";
	fprintf(stdout, "%s start...\n", tag);


	pthread_t tids[tcnt];
	for (int i = 0; i < tcnt; ++i) {
		errno = pthread_create(&tids[i], NULL, __test003_worker, (void *)(long)i);
		if (errno) {
			fprintf(stdout, "%s create the %dth thread failed: %m\n", tag, i);
			exit(1);
		}
	}

	for (int i = 0; i < tcnt; ++i) {
		pthread_join(tids[i], NULL);
	}

	fprintf(stdout, "%s done\n", tag);
	return;
}

static void test004_mt_async_req_rsp()
{
	static const char *tag = "test004: mt-async-req-rsp";
	fprintf(stdout, "%s start...\n", tag);

	// TODO: ...

	fprintf(stdout, "%s done\n", tag);
	return;
}


int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s host [thread-cnt]\n", argv[0]);
		exit(0);
	}

	srandom(time(NULL));	
	setbuf(stdout, NULL);

	host = argv[1];
	if (argc >= 3) tcnt = atoi(argv[2]);

	test001_sync_req_rsp();
	test002_async_req_rsp();
	test003_mt_sync_req_rsp();
	test004_mt_async_req_rsp();

	return 0;
}
