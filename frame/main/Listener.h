/* Listener.h
 *
 * 1, created by Robert Yu in 2016 March
**/
#ifndef __LISTENER__H
#define __LISTENER__H

#include "EventHandler.h"

namespace beyondy {
namespace Async {

class EventManager;
class Message;
template<class T> class Queue;
class Processor;
class Connection;

class Listener : public EventHandler {

public:
	Listener(const char *address, int connMax, EventManager *emgr, Queue<Message> *inQueue, Processor *proc);
	~Listener();
public:
	virtual int onReadable(int fd);
	virtual int onWritable(int fd);
	virtual int onError(int fd);
public:
	Connection *createConnection(int fd);
	void destroyConnection(Connection *connection);
public:
	int open();
private:
	char address[128];

	int connActive;
	int connMax;
	
	EventManager *emgr;
	Queue<Message> *inQueue;
	Processor *proc;
};

} /* Async */
} /* beyondy */

#endif /* __LISTENER__H */
