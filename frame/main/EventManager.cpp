#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdexcept>
#include <errno.h>
#include <assert.h>

#include "beyondy/bprof.h"
#include "Bprof_ids.h"
#include "Queue.h"
#include "Listener.h"
#include "Connection.h"
#include "Connector.h"
#include "Message.h"
#include "EventManager.h"
#include "Log.h"

namespace beyondy {
namespace Async {

EventManager::EventManager(Queue<Message> *_outQueue, int _esize)
	: epoll_fd(-1), event_next(0), event_total(0), epoll_size(_esize), epoll_events(NULL),
	  outQueue(_outQueue)
{
	if (epoll_size < 1) epoll_size = 1024;

	if ((epoll_fd = epoll_create(epoll_size)) < 0) {
		char buf[512];
		snprintf(buf, sizeof buf, "epoll_create(%ld) failed: %m", (long)epoll_size);
		throw std::runtime_error(buf);
	}

	epoll_events = (struct epoll_event *)malloc(epoll_size * sizeof(struct epoll_event));
	if (epoll_events == NULL) {
		::close(epoll_fd);

		char buf[512];
		snprintf(buf, sizeof buf, "new epoll events[%ld] failed: %m", (long)epoll_size);
		throw std::runtime_error(buf);
	}

	event_next = 0;
	event_total = 0;

	INIT_LIST_HEAD(&lruHead);
	INIT_LIST_HEAD(&connectorsHead);
		
	struct rlimit r0;
	int retval = getrlimit(RLIMIT_NOFILE, &r0);
	if (retval < 0) {
		throw std::runtime_error("rlimit(NOFILE) failed");
	}

	next_flow = 1;
	flowRoot.rb_node = NULL;

	perCheckOutQueue = 100;
	connectionMaxIdle = 5 * 1000;
	reconnectDelay = 2 * 1000;	// will delay 2, 4, 6, 8, 10
}

int EventManager::addListener(const char *address, Queue<Message> *inQueue, Processor *processor, int maxActive)
{
	Listener *listener = new Listener(address, maxActive, this, inQueue, processor);
	if (listener == NULL) {
		SYSLOG_ERROR("open listener at %s failed", address);
		return -1;
	}
	
	if (listener->open() < 0 || this->addHandler(listener, EVENT_IN) < 0) {
		SYSLOG_ERROR("open or watch-IN failed for listener at %s", address);
		delete listener;
		return -1;
	}

	return 0;
}

int EventManager::addConnector(const char *address, Queue<Message> *inQueue, Processor *processor, int openImmediately)
{
	int flow = nextFlow();
	Connector *connector = new Connector(flow, address, this, inQueue, processor);

	if (openImmediately && connector->open() < 0) {
		SYSLOG_ERROR("open Connector to %s failed", address);
		return -1;
	}

	mapFlow(flow, connector);
	_list_add_tail(&connector->connectorsEntry, &connectorsHead);

	return flow;
}

int EventManager::addHandler(EventHandler *handler, int events)
{
	struct epoll_event ee;
	int retval;
	
	ee.events = events;
	ee.data.ptr = (void *)handler;
	
	retval = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->fd, &ee);
	if (retval == 0) {
		handler->events = events;
		SYSLOG_DEBUG("addHandler fd=%d events=%d OK", handler->fd, events);
	}
	else {
		SYSLOG_DEBUG("addHandler fd=%d failed: %m", handler->fd);
	}

	return retval;
}

int EventManager::addConnection(Connection *connection, int events)
{
	if (addHandler(connection, events) < 0) {
		SYSLOG_ERROR("can not add connection fd=%d flow=%d", connection->fd, connection->flow);
		return -1;
	}

	/* init it */
	gettimeofday(&connection->tsLastRead, NULL);
	_list_add_tail(&connection->lruEntry, &lruHead);

	SYSLOG_DEBUG("addConneciton fd=%d flow=%d OK", connection->fd, connection->flow);
	return 0;
}

int EventManager::modifyConnection(Connection *connection, int add, int remove)
{
	struct epoll_event ee;
	int retval;

	ee.events = (connection->events | add) & ~remove;
	ee.data.ptr = (void *)(EventHandler *)connection;

	retval = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, connection->fd, &ee);
	if (retval == 0) {
		connection->events = ee.events;
		SYSLOG_DEBUG("modifyConneciton fd=%d flow=%d events=%d OK", connection->fd, connection->flow, ee.events);
	}
	else {
		SYSLOG_DEBUG("modifyConneciton fd=%d flow=%d events=%d failed: %m", connection->fd, connection->flow, ee.events);
	}

	return retval;
}

int EventManager::deleteConnection(Connection *connection)
{
	struct epoll_event ee;
	int retval;

	ee.events = 0;
	ee.data.ptr = (void *)(EventHandler *)connection;

	if (!(retval = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connection->fd, &ee))) {
		_list_del(&connection->lruEntry);

		SYSLOG_ERROR("delete connection fd=%d flow=%d", connection->fd, connection->flow);
	}
	else {
		SYSLOG_ERROR("delete connection fd=%d flow=%d failed: %m", connection->fd, connection->flow);
	}
	
	return retval;
}

int EventManager::nextFlow()
{
	int flow = next_flow++;
	return flow;
}

Connection *EventManager::flow2Connection(int flow)
{
	struct rb_node *n = flowRoot.rb_node;
	Connection *connection;

	while (n != NULL) {
		connection = rb_entry(n, Connection, flowEntry);
		if (flow < connection->flow)
			n = n->rb_left;
		else if (flow > connection->flow)
			n = n->rb_right;
		else
			return connection;
	}

	return NULL;
}

Connection *EventManager::__insert(int flow, struct rb_node *node)
{
	struct rb_node **p = &flowRoot.rb_node;
	struct rb_node *parent = NULL;
	struct Connection *connection;

	while (*p != NULL) {
		parent = *p;
		connection = rb_entry(parent, Connection, flowEntry);
		if (flow > connection->flow)
			p = &(*p)->rb_right;
		else if (flow < connection->flow)
			p = &(*p)->rb_left;
		else
			return connection;
	}

	rb_link_node(node, parent, p);
	return NULL;
}

int EventManager::mapFlow(int flow, Connection *connection)
{
	assert(flow2Connection(flow) == NULL);
	Connection *ret = __insert(flow, &connection->flowEntry);
	if (ret == NULL) rb_insert_color(&connection->flowEntry, &flowRoot);

	SYSLOG_DEBUG("mapFlow=%d => %d", flow, connection->fd);
	return 0;
}

int EventManager::unmapFlow(Connection *connection)
{
	assert(flow2Connection(connection->flow) != NULL);
	SYSLOG_DEBUG("upmapFlow=%d => %d", connection->flow, connection->fd);
		
	rb_erase(&connection->flowEntry, &flowRoot);
	return 0;
}

int EventManager::waitingEvents()
{
	BPROF_TRACE(BPT_EM_WAITING)
	// TODO: wtime = min(gap-to-next-timer, 10ms)
	long wtime = 10; /* 1s */

	event_next = event_total = 0;
	int cnt = epoll_wait(epoll_fd, epoll_events, epoll_size, wtime);
	if (cnt >= 0) event_total = cnt;

	return cnt;
}

void EventManager::handleEvent(int fd, EventHandler *handler, int event)
{
	BPROF_TRACE(BPT_EM_HEVENT)
	int retval = 0;

	if ((event & EVENT_OUT) && (retval = handler->onWritable(fd)) < 0) {
		SYSLOG_ERROR("fd=%d onWritable failed", handler->fd);
	}

	if (retval >= 0 && (event & EVENT_IN) && (retval = handler->onReadable(fd)) < 0) {
		SYSLOG_ERROR("fd=%d onReadable failed", handler->fd);
	}

	if (retval < 0 || (event & EVENT_ERR)) {
		SYSLOG_ERROR("fd=%d got retval=%d or ERR event=%d", fd, retval, event);
		retval = handler->onError(fd);
		// Note!!!
		// handler is not available now
	}

	return;
}

void EventManager::handleEvents()
{
	BPROF_TRACE(BPT_EM_HEVENTS)

	for (event_next = 0; event_next < event_total; ++event_next) {
		struct epoll_event *ee = &epoll_events[event_next];
		EventHandler *handler = static_cast<EventHandler *>(ee->data.ptr);
		handleEvent(handler->fd, handler, ee->events);
	}

	return;
}

void EventManager::dispatchOutMessage()
{
	BPROF_TRACE(BPT_EM_DISPATCH)

	for (int i = 0; i < perCheckOutQueue; ++i) {
		Message *msg = outQueue->pop();
		if (msg == NULL)
			break;

		gettimeofday(&msg->ts_dequeue, NULL);
		int flow = msg->flow;

		Connection *connection = flow2Connection(flow);
		if (connection == NULL) {
			SYSLOG_ERROR("can not find connection for fd=%d flow=%d discard it", msg->fd, msg->flow);
			// TODO: how to call proc-onSent(msg, SS_SOCKET_NON_EXIST)
			Message::destroy(msg);
			continue;
		}
		
		int retval = connection->sendMessage(msg);
		if (retval != 0) {
			// msg has been freed
			SYSLOG_ERROR("sendMessage to fd=%d flow=%d failed", connection->fd, flow);
			continue;
		}
	}
}

void EventManager::checkTimeout()
{
	BPROF_TRACE(BPT_EM_CHKTIMEOUT)
	struct timeval tNow;
	struct list_head *pl, *pn;

	gettimeofday(&tNow, NULL);
	list_for_each_safe(pl, pn, &lruHead) {
		Connection *connection = list_entry(pl, Connection, lruEntry);
		if (TV2MS(connection->tsLastRead, tNow) > connectionMaxIdle) {
			SYSLOG_ERROR("connection fd=%d flow=%d timed out. close it now.", connection->fd, connection->flow);
			connection->onError(connection->fd);
			continue;
		}

		// the rest should be more active
		// so no need check this time.
		break;
	}
}

void EventManager::reconnectWhenNecessary()
{
	struct timeval tNow;
	struct list_head *pl, *pn;

	gettimeofday(&tNow, NULL);
	list_for_each_safe(pl, pn, &connectorsHead) {
		Connector *connector = list_entry(pl, Connector, connectorsEntry);
		if (connector->getStatus() == Connector::CONN_CLOSE
				&& TV2MS(connector->tsLastClosed, tNow) > (connector->cntRetryConnect + 1) * reconnectDelay) {
			SYSLOG_ERROR("connector flow=%d is closed, re-connecting it", connector->flow);
			if (connector->open() < 0) {
				SYSLOG_WARN("re-connecting for low=%ld failed: %m", connector->flow);
				// retry next time but update tsLastClosed to avoid connect too often
			}

		}

		connector->checkMessageExpiration();
	}

	return;
}

int EventManager::start()
{
	/* start it */
	isRunning = 1;

	while (isRunning) {
		BPROF_TRACE(BPT_LOOP)

		waitingEvents();

		handleEvents();

		// check outQueue
		dispatchOutMessage();

		// timeout check
		checkTimeout();

		// re-connect for connectors if they are broken
		reconnectWhenNecessary();
	}

	return 0;
}

#if 0 //LEADER_THREADS
int EventManager::start() {
	pts = leader_threads_create(10, EventManager::LeaderEntry, this);
}

void EventManager::LeaderEntry(leader_threads_t *pts, void *data) {
	EventManager *emgr = dynamic_cast<EventManager *>data;
	emgr->leader();
}

int EventManager::leader() {

	if (enext >= etotal) {
		waitingForEvents();
	}

	while (enext < etotal) {
		struct epoll_event *event = events[enext++];
		int status = HandleEvent(ehandler, event.event);
		if (status == NO_LEADER_ANY_MORE) {
			/* return and need to be leader again */
			return 0;
		}
	}			
}
#endif

} /* Async */
} /* beyondy */

