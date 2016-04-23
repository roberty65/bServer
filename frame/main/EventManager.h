#ifndef __EVENT_MANAGER__H
#define __EVENT_MANAGER__H

#include <sys/epoll.h>
#include <map>

#include "beyondy/list.h"
#include "beyondy/rbtree.h"

namespace beyondy {
namespace Async {

#define EVENT_IN	EPOLLIN
#define EVENT_OUT	EPOLLOUT
#define EVENT_ERR	EPOLLERR


class Message;
template<class T> class Queue;
class Processor;
class EventHandler;
class Connection;

class EventManager {
public:
	EventManager(Queue<Message> *outQueue, int esize);
public:
	// must be called be start is called by ONE thread
	// No deletion
	int addListener(const char *address, Queue<Message> *inQueue, Processor *processor, int maxActive);
	int addConnector(const char *address, Queue<Message> *inQueue, Processor *process, int openImmediately);
private:
	//internal use, no lock?!
	int addHandler(EventHandler *handler, int events);
public:
	int addConnection(Connection *connection, int events);
	int modifyConnection(Connection *connection, int add, int remove);
	int deleteConnection(Connection *connection);
private:
	Connection *__insert(int flow, struct rb_node *node);
public:
	int nextFlow();
	int mapFlow(int flow, Connection *connection);
	int unmapFlow(Connection *connection);
private:
	int waitingEvents();
	void handleEvents();
	void handleEvent(int fd, EventHandler *handler, int event);
	Connection *flow2Connection(int flow);
	void dispatchOutMessage();
	void checkTimeout();
	void reconnectWhenNecessary();
public:
	int start();
	void setConnectionMaxIdle(int seconds) { this->connectionMaxIdle = seconds * 1000; }
	void setReconnectDelay(int seconds) { this->reconnectDelay = seconds * 1000; }
private:
	int epoll_fd;

	int event_next;
	int event_total;
	
	int epoll_size;
	struct epoll_event *epoll_events;

	Queue<Message> *outQueue;
	
	/* LRU list */
	struct list_head lruHead;
	struct list_head connectorsHead;
	
	int next_flow;
	struct rb_root flowRoot;	/* flow => connection */

	int isRunning;

	int perCheckOutQueue;
	int connectionMaxIdle;		/* seconds */
	int reconnectDelay;		/* second */
	
};
} /* Async */
}/* beyondy */

#endif /*__EVENT_MANAGER__H */
