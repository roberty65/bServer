#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include "Block.h"
#include "Message.h"
#include "Queue.h"
#include "Processor.h"
#include "MtWorker.h"
#include "Log.h"

namespace beyondy {
namespace Async {

MtWorker::MtWorker(Queue<Message> *_inQueue, Processor *_proc, int _tcnt)
	: inQueue(_inQueue), proc(_proc), isRunning(0), tcnt(_tcnt)
{
	tids = new pthread_t[tcnt];
	for (int i = 0; i < tcnt; ++i) {
		tids[i] = 0;
	}
}

MtWorker::~MtWorker()
{
	/* nothing */
}

void *MtWorker::__workerEntry(void *p)
{
	MtWorker *mtWorker = (MtWorker *)p;
	mtWorker->workerEntry();
	return NULL;
}

int MtWorker::workerId()
{
	int no = -1;

	for (int i = 0; i < tcnt; ++i) {
		if (pthread_equal(tids[i], pthread_self())) {
			no = i;
			break;
		}
	}

	return no;
}

void MtWorker::workerEntry()
{
	int no = workerId();
	SYSLOG_DEBUG("thread-%d (TID=%lu) start...", no, (unsigned long)pthread_self());

	while (isRunning) {
		Message *msg = inQueue->pop(10);
		if (msg == NULL) {
			// TODO: idle
			continue;
		}

		gettimeofday(&msg->ts_dequeue, NULL);
		proc->onMessage(msg);
	}
}

int MtWorker::start()
{
	isRunning = 1;
	for (int i = 0; i < tcnt; ++i) {
		int retval = pthread_create(&tids[i], NULL, __workerEntry, (void *)this);
		if (retval != 0) {
			errno = retval;
			return -1;
		}
	}

	return 0;
}

int MtWorker::stop()
{
	isRunning = 0;
	for (int i = 0; i < tcnt; ++i) {
		pthread_join(tids[i], NULL);
	}

	return 0;
}

} /* Async */
} /* beyondy */

