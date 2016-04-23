#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "beyondy/bprof.h"

namespace beyondy {

struct bprof_item *bp_items = 0;
size_t bp_size = 0;

struct bprof_item *bk_items = 0;
struct bprof_item *cc_items = 0;

char *ff_buf;
size_t ff_size = 0;

static pthread_t bp_tid;
static void *bprof_outer(void *p)
{
	while (true) {
		sleep(10);
		// snapshot
		memcpy(bk_items, bp_items, sizeof(struct bprof_item) * bp_size);
		time_t tnow; time(&tnow);
		struct tm tmbuf, *ptm = localtime_r(&tnow, &tmbuf);

		int offset = snprintf(ff_buf, ff_size, "%04d-%02d-%02d %02d:%02d:%02d\n",
				ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

		for (size_t i = 0; i < bp_size; ++i) {
			cc_items[i].count = bk_items[i].count - cc_items[i].count;
			cc_items[i].usec = bk_items[i].usec - cc_items[i].usec;
			unsigned long avg = cc_items[i].count ? cc_items[i].usec/cc_items[i].count : 0;
			offset += snprintf(ff_buf + offset, ff_size - offset,
					"%32s cnt=%lu total-usec=%lu avg=%lu\n",  	
					bp_items[i].name, cc_items[i].count, 
					cc_items[i].usec, avg);
		}

		if (offset >= (int)ff_size - 1) {
			ff_buf[ff_size - 1] = 0;
			offset = ff_size - 1;
		}

		// write file
		int fd = open("../logs/bprof.out", O_CREAT|O_APPEND|O_WRONLY, 0664);
		if (fd < 0) {
			fprintf(stderr, "%s\n", ff_buf);
		}
		else {
			ssize_t wlen = write(fd, ff_buf, offset);
			assert(wlen == offset);

			close(fd);
		}

		// save backup
		memcpy(cc_items, bk_items, sizeof(struct bprof_item) * bp_size);	
	}

	return NULL;
}

int bprof_init(struct bprof_item *items, size_t size)
{
	bp_items = items;
	bp_size = size;

	bk_items = new struct bprof_item[size];
	cc_items = new struct bprof_item[size];

	ff_size = size * (80 + 4 * 64) + 64;
	ff_buf = new char[ff_size];

	errno = pthread_create(&bp_tid, NULL, bprof_outer, NULL);
	return errno ? -1 : 0;
}

void bprof_exit()
{
	pthread_kill(bp_tid, SIGINT);
	pthread_join(bp_tid, NULL);
}

} /* beyondy */

