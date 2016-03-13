#ifndef __CONNECTION__H
#define __CONNECTION__H

#include "EventHandler.h"
#include "beyondy/list.h"

namespace beyondy {
namespace Async {

#define CT_LISTENER		0
#define CT_CONNECTOR		1
#define CT_BALANCED_CONNECTOR	2

class EventManager;
class Processor;
class Listener;
template<class T> class Queue;
class Message;

class Connection : public EventHandler {
public:
	Connection(int fd, int flow, EventManager *emgr, Queue<Message> *inQueue, Processor *proc, int _creatorType, void *parent);
public:
	virtual int onReadable(int fd);
	virtual int onWritable(int fd);
	virtual int onError(int fd);
public:
	virtual int sendMessage(Message *msg);
public:
	virtual int destroy();
private:
	ssize_t readN(size_t size);
	ssize_t writeN(Message *msg, size_t size);
public: // getter??
	int flow;
	struct list_head lruEntry;	// LRU entry. must be public
	struct timeval tsLastRead;
protected:
	Message *inMsg;
	Queue<Message> *inQueue;	// shared

	Message *outMsg;
	Queue<Message> *outQueue;	// delicated for out messages(NO NEED LOCK?)

	EventManager *emgr;
	Processor *proc;

	int creatorType;
	union {
		Listener *__listener;
		void *__data;
	} __creator;
#define parentListener __creator.__listener
#define parentData __creator.__data
};

} /* Async */
} /* beyondy */

#endif /* __CONNECTION__H */
