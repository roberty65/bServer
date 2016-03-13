#ifndef __MT_WORKER__H
#define __MT_WORKER__H

#include <pthread.h>

namespace beyondy {
namespace Async {

class Message;
template <class T> class Queue;
class Processor;

class MtWorker {
public:
	MtWorker(Queue<Message> *inQueue, Processor *proc, int tcnt);
	~MtWorker();
private:
	MtWorker(const MtWorker&);
	MtWorker& operator=(const MtWorker&);
private:
	static void *__workerEntry(void *p);
	int workerId();
	void workerEntry();
public:
	int start();
	int stop();
private:
	Queue<Message> *inQueue;
	Processor *proc;

	volatile int isRunning;
	int tcnt;
	pthread_t *tids;
};

} /* Async */
} /* beyondy */
#endif /*__MT_WORKER__H */

